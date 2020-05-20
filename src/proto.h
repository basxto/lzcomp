#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define COMPRESSION_METHODS         96 /* sum of all values for the methods field in compressors */
#define MAX_FILE_SIZE            32768
#define SHORT_COMMAND_COUNT         32
#define MAX_COMMAND_COUNT         1024
#define LOOKBACK_LIMIT             128 /* highest negative valid count for a copy command */
#define LOOKAHEAD_LIMIT           3072 /* maximum lookahead distance for the first pass of multi-pass compression */
#define MULTIPASS_SKIP_THRESHOLD    64

struct command {
  unsigned command: 3; // commands 0-6 as per compression spec; command 7 is used as a dummy placeholder
  unsigned count:  12; // always equals the uncompressed data length
  signed value:    17; // offset for commands 0 (into source) and 4-6 (into decompressed output); repeated bytes for commands 1-2
};

struct compressor {
  unsigned methods;
  struct command * (* function) (const unsigned char *, const unsigned char *, unsigned short *, unsigned);
};

struct options {
  const char * input;
  const char * output;
  unsigned method;
  unsigned char mode;
  unsigned char alignment;
};

// global.c
extern const struct compressor compressors[];

// main.c
int main(int, char **);
struct command * compress(const unsigned char *, unsigned short *);
struct command * compress_single_method(const unsigned char *, unsigned short *, unsigned);

// merging.c
struct command * select_command_sequence(struct command **, const unsigned short *, unsigned, unsigned short *);
struct command * merge_command_sequences(const struct command *, unsigned short, const struct command *, unsigned short, unsigned short *);

// mpcomp.c
struct command * try_compress_multi_pass(const unsigned char *, const unsigned char *, unsigned short *, unsigned);
struct command pick_command_for_pass(const unsigned char *, const unsigned char *, const unsigned char *, const short *, unsigned short,
                                     unsigned short, unsigned);
struct command pick_repetition_for_pass(const unsigned char *, unsigned short, unsigned short, unsigned);
struct command pick_copy_for_pass(const unsigned char *, const unsigned char *, const short *, unsigned char, unsigned short, unsigned short, unsigned);

// nullcomp.c
struct command * store_uncompressed(const unsigned char *, const unsigned char *, unsigned short *, unsigned);

// options.c
struct options get_options(int, char **);
unsigned parse_numeric_option_argument(char ***, unsigned);
void usage(const char *);

// output.c
void write_commands_to_textfile(const char *, const struct command *, unsigned, const unsigned char *, unsigned char);
void write_command_to_textfile(FILE *, struct command, const unsigned char *);
void write_commands_to_file(const char *, const struct command *, unsigned, const unsigned char *, unsigned char);
void write_command_to_file(FILE *, struct command, const unsigned char *);

// packing.c
void optimize(struct command *, unsigned short);
void repack(struct command **, unsigned short *);

// repcomp.c
struct command * try_compress_repetitions(const unsigned char *, const unsigned char *, unsigned short *, unsigned);
struct command find_repetition_at_position(const unsigned char *, unsigned short, unsigned short);

// spcomp.c
struct command * try_compress_single_pass(const unsigned char *, const unsigned char *, unsigned short *, unsigned);
struct command find_best_copy(const unsigned char *, unsigned short, unsigned short, const unsigned char *, unsigned);
unsigned short scan_forwards(const unsigned char *, unsigned short, const unsigned char *, unsigned short, short *);
unsigned short scan_backwards(const unsigned char *, unsigned short, unsigned short, short *);
struct command find_best_repetition(const unsigned char *, unsigned short, unsigned short);

// util.c
void error_exit(int, const char *, ...);
unsigned char * read_file_into_buffer(const char *, unsigned short *);
void bit_flip(const unsigned char *, unsigned short, unsigned char *);
struct command pick_best_command(unsigned, struct command, ...);
int is_better(struct command, struct command);
short command_size(struct command);
unsigned short compressed_length(const struct command *, unsigned short);
