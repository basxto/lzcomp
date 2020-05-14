#include "proto.h"

struct command * try_compress (const unsigned char * data, const unsigned char * bitflipped, unsigned short * length, unsigned flags) {
  struct command * commands = malloc(sizeof(struct command) * *length);
  memset(commands, -1, sizeof(struct command) * *length);
  struct command * current_command = commands;
  unsigned short position = 0, previous_data = 0;
  unsigned char lookahead = 0, lookahead_flag = (flags >> 3) % 3;
  struct command copy, repetition;
  while (position < *length) {
    copy = find_best_copy(data, position, *length, bitflipped, flags);
    repetition = find_best_repetition(data, position, *length);
    if (flags & 1)
      *current_command = pick_best_command(2, repetition, copy);
    else
      *current_command = pick_best_command(2, copy, repetition);
    *current_command = pick_best_command(2, (struct command) {.command = 0, .count = 1, .value = position}, *current_command);
    if (flags & 2) {
      if (previous_data && (previous_data != 32) && (previous_data != 1024) && (command_size(*current_command) == current_command -> count))
        *current_command = (struct command) {.command = 0, .count = 1, .value = position};
    }
    if (lookahead_flag) {
      if (lookahead >= lookahead_flag)
        lookahead = 0;
      else if (current_command -> command) {
        lookahead ++;
        *current_command = (struct command) {.command = 0, .count = 1, .value = position};
      }
    }
    if (current_command -> command)
      previous_data = 0;
    else
      previous_data += current_command -> count;
    position += (current_command ++) -> count;
  }
  optimize(commands, current_command - commands);
  repack(&commands, length);
  return commands;
}

struct command find_best_copy (const unsigned char * data, unsigned short position, unsigned short length, const unsigned char * bitflipped, unsigned flags) {
  struct command simple = {.command = 7};
  struct command flipped = simple, backwards = simple;
  short count, offset;
  if (count = scan_forwards(data + position, length - position, data, position, &offset))
    simple = (struct command) {.command = 4, .count = count, .value = offset};
  if (count = scan_forwards(data + position, length - position, bitflipped, position, &offset))
    flipped = (struct command) {.command = 5, .count = count, .value = offset};
  if (count = scan_backwards(data, length - position, position, &offset))
    backwards = (struct command) {.command = 6, .count = count, .value = offset};
  struct command command;
  switch (flags / 24) {
    case 0: command = pick_best_command(3, simple, backwards, flipped); break;
    case 1: command = pick_best_command(3, backwards, flipped, simple); break;
    case 2: command = pick_best_command(3, flipped, backwards, simple);
  }
  if ((flags & 4) && (command.count > 32)) command.count = 32;
  return command;
}

unsigned short scan_forwards (const unsigned char * target, unsigned short limit, const unsigned char * source, unsigned short real_position, short * offset) {
  unsigned short best_match, best_length = 0;
  unsigned short current_length;
  unsigned short position;
  for (position = 0; position < real_position; position ++) {
    if (source[position] != *target) continue;
    for (current_length = 0; (current_length < limit) && (source[position + current_length] == target[current_length]); current_length ++);
    if (current_length > 1024) current_length = 1024;
    if (current_length < best_length) continue;
    best_match = position;
    best_length = current_length;
  }
  if (!best_length) return 0;
  if ((best_match + 128) >= real_position)
    *offset = best_match - real_position;
  else
    *offset = best_match;
  return best_length;
}

unsigned short scan_backwards (const unsigned char * data, unsigned short limit, unsigned short real_position, short * offset) {
  if (real_position < limit) limit = real_position;
  unsigned short best_match, best_length = 0;
  unsigned short current_length;
  unsigned short position;
  for (position = 0; position < real_position; position ++) {
    if (data[position] != data[real_position]) continue;
    for (current_length = 0; (current_length <= position) && (current_length < limit) &&
                             (data[position - current_length] == data[real_position + current_length]); current_length ++);
    if (current_length > 1024) current_length = 1024;
    if (current_length < best_length) continue;
    best_match = position;
    best_length = current_length;
  }
  if (!best_length) return 0;
  if ((best_match + 128) >= real_position)
    *offset = best_match - real_position;
  else
    *offset = best_match;
  return best_length;
}

struct command find_best_repetition (const unsigned char * data, unsigned short position, unsigned short length) {
  if ((position + 1) >= length) return data[position] ? ((struct command) {.command = 7}) : ((struct command) {.command = 3, .count = 1});
  unsigned char value[2] = {data[position], data[position + 1]};
  unsigned repcount, limit = length - position;
  if (limit > 1024) limit = 1024;
  for (repcount = 2; (repcount < limit) && (data[position + repcount] == value[repcount & 1]); repcount ++);
  struct command result;
  result.count = repcount;
  if (*value != value[1]) {
    if (!*value && (repcount < 3)) return (struct command) {.command = 3, .count = 1};
    result.command = 2;
    result.value = ((unsigned) (*value)) | (((unsigned) (value[1])) << 8);
  } else if (*value) {
    result.command = 1;
    result.value = *value;
  } else
    result.command = 3;
  return result;
}
