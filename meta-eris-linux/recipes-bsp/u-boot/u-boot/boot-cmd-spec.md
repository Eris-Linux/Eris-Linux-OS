# Specifications for boot-cmd script.

## Finite State Machine.

The Eris Linux boot script is based on a FSM (Finite State Machine).

### Pseudo-code

```

ready_to_boot_on = 0

if boot_state == "":
	boot_state = ERIS_BOOT_STATUS_STABLE
	confirmed_partition = ERIS_SYSTEM_A_PART
	boot_count = ERIS_MAX_BOOT_ATTEMPTS

if boot_state == ERIS_BOOT_STATUS_STABLE:
	if boot_count > 0:
		boot_count--
		saveenv
		ready_to_boot_on = confirmed_partition
	else:
		boot_state = ERIS_BOOT_STATUS_REPAIR
		boot_count = ERIS_MAX_BOOT_ATTEMPTS
		recovery_done = false
		saveenv

if boot_state == ERIS_BOOT_STATUS_TRIAL:
	if boot_count > 0:
		boot_count--
		saveenv
		boot_on(candidate_partition)
	else:
		boot_state = ERIS_BOOT_STATUS_ROLLBACK
		boot_count = ERIS_MAX_BOOT_ATTEMPTS
		recovery_done = false
		saveenv
		save_failure_event(candidate_partition)

if boot_state == ERIS_BOOT_STATUS_ROLLBACK:
	if recovery_done == false:
		restore_save_data()
		recovery_done = true
	if boot_count > 0:
		boot_count--
		saveenv
		boot_on(confirmed_partition)
	else:
		boot_state = ERIS_BOOT_STATUS_REPAIR
		boot_count = ERIS_MAX_BOOT_ATTEMPTS
		recovery_done = false
		saveenv

if boot_state == ERIS_BOOT_STATUS_REPAIR:
	if recovery_done == false:
		repair_data()
		recovery_done = true
		saveenv
	if boot_count > 0:
		boot_count--
		saveenv
		boot_on(confirmed partition)
	else:
		erase_all_data()
		boot_count = ERIS_MAX_BOOT_ATTEMPTS
		boot_state = ERIS_BOOT_STATUS_RESET
		recovery_done = false

if boot_state == ERIS_BOOT_STATUS_RESET:
	if boot_count > 0:
		boot_count--
		saveenv
		boot_on(confirmed partition)
	else:
		??????
```	


### Linux script when boot ok

```
confirmed_partition = candidate_partition;
candidate_partition = NONE;
boot_state = ERIS_BOOT_STATUS_STABLE;
boot_count = 3;
```
