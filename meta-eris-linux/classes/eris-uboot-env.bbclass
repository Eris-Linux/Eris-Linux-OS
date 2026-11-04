# Validate once for every consumer, before generating any build artefacts.
ERIS_UBOOT_ENV_LAYOUT_VARS = "ERIS_UBOOT_ENV_RAW ERIS_UBOOT_ENV_DEVICE ERIS_UBOOT_ENV_MMC_DEVICE ERIS_UBOOT_ENV_OFFSET ERIS_UBOOT_ENV_OFFSET_REDUND ERIS_UBOOT_ENV_SIZE ERIS_UBOOT_BOOT_OFFSET"
# The helper constructs variable names, so declare these dependencies explicitly.
do_configure[vardeps] += "${ERIS_UBOOT_ENV_LAYOUT_VARS}"
do_install[vardeps] += "${ERIS_UBOOT_ENV_LAYOUT_VARS}"
do_deploy[vardeps] += "${ERIS_UBOOT_ENV_LAYOUT_VARS}"

def eris_uboot_env_layout(d):
    values = {}
    for key in ('OFFSET', 'OFFSET_REDUND', 'SIZE', 'MMC_DEVICE'):
        values[key] = int(d.getVar('ERIS_UBOOT_ENV_' + key), 0)
    values['BOOT_OFFSET'] = int(d.getVar('ERIS_UBOOT_BOOT_OFFSET'), 0)
    values['DEVICE'] = d.getVar('ERIS_UBOOT_ENV_DEVICE')

    for key in ('OFFSET', 'OFFSET_REDUND', 'SIZE', 'BOOT_OFFSET'):
        if values[key] <= 0 or values[key] % 1024:
            raise ValueError('%s must be positive and aligned to 1 KiB' % key)
    if values['OFFSET'] < 0x100000:
        raise ValueError('keep the first MiB free for the MBR and boot loaders')
    if values['OFFSET'] + values['SIZE'] > values['OFFSET_REDUND']:
        raise ValueError('the two environment copies overlap or are reversed')
    if values['OFFSET_REDUND'] + values['SIZE'] > values['BOOT_OFFSET']:
        raise ValueError('the redundant environment overlaps BOOT')
    if values['BOOT_OFFSET'] % 0x400000:
        raise ValueError('BOOT must be aligned to 4 MiB, as required by the WKS files')
    if values['MMC_DEVICE'] < 0:
        raise ValueError('MMC_DEVICE must be a nonnegative MMC index')
    import re
    if not re.fullmatch(r'/dev/mmcblk[0-9]+', values['DEVICE'] or ''):
        raise ValueError('DEVICE must name the whole Linux MMC user-area device')
    return values

def eris_uboot_env_config(d):
    if d.getVar('ERIS_UBOOT_ENV_RAW') != '1':
        return '/boot/uboot.env 0x0 %s\n' % d.getVar('ERIS_UBOOT_ENV_SIZE')
    layout = eris_uboot_env_layout(d)
    return ('# MMC user area: device, byte offset, environment size\n' +
            ''.join('%s 0x%x 0x%x\n' %
                    (layout['DEVICE'], layout[key], layout['SIZE'])
                    for key in ('OFFSET', 'OFFSET_REDUND')))

def eris_uboot_env_fragment(d):
    layout = eris_uboot_env_layout(d)
    return '\n'.join((
        'CONFIG_ENV_IS_IN_MMC=y',
        '# CONFIG_ENV_IS_IN_FAT is not set',
        'CONFIG_SYS_REDUNDAND_ENVIRONMENT=y',
        'CONFIG_SYS_MMC_ENV_PART=0',
        'CONFIG_SYS_MMC_ENV_DEV=%d' % layout['MMC_DEVICE'],
        'CONFIG_ENV_SIZE=0x%x' % layout['SIZE'],
        'CONFIG_ENV_OFFSET=0x%x' % layout['OFFSET'],
        'CONFIG_ENV_OFFSET_REDUND=0x%x' % layout['OFFSET_REDUND'],
        '# CONFIG_ENV_MMC_USE_DT is not set',
        '# CONFIG_USE_ENV_MMC_PARTITION is not set',
        ''))

python __anonymous() {
    if d.getVar('ERIS_UBOOT_ENV_RAW') == '1':
        try:
            eris_uboot_env_layout(d)
        except (ValueError, TypeError) as error:
            bb.fatal('Invalid Eris MMC environment layout: %s' % error)
}
