/* SPDX-License-Identifier: MIT

   Christophe BLAESS 2025.
   Copyright 2025 Logilin. All rights reserved.
*/

#ifndef ERIS_API_TEST_H
#define ERIS_API_TEST_H

/// @brief Format and write data to a socket.
/// @param sockfd The socket to write data to.
/// @param format printf-like string format.
/// @return The number of bytes written, -1 on error.
int   sockprintf(int sockfd, const char *format, ...);

/// @brief Read a line from a socket.
/// @param sockfd The soket to read data from.
/// @param buffer The buffer to fill.
/// @param size The size of the buffer.
/// @return A pointer to the buffer, NULL on error (like fgets()).
char *sockgets(int sockfd, char *buffer, size_t size);

#define BUFFER_SIZE       2048

#endif