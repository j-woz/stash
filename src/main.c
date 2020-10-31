
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "stash.h"

#include "stash_log.h"

static void get_flags(int argc, char* argv[]);

static void help(void);

int
main(int argc, char* argv[])
{
  stash_init();

  if (argc == 1)
  {
    printf("stash: No arguments!\n");
    return 0;
  }

  get_flags(argc, argv);

  if (optind + 2 > argc)
  {
    help();
    printf("\n");
    stash_abort("provide more arguments!\n");
  }

  char* subcmd_text = argv[optind];
  stash_subcmd subcmd;
  bool rc;
  rc = stash_subcmd_lookup(subcmd_text, &subcmd);
  if (!rc) stash_abort("No such subcommand: %s", subcmd_text);

  char* text_file = argv[optind+1];

  char* hunks = NULL;
  if (argc > optind+2)
    hunks = argv[optind+2];

  rc = stash_init_tmp();
  if (!rc) stash_abort("could not initialize");

  if (subcmd == STASH_SUBCMD_PUSH)
    rc = stash_push(text_file, hunks);
  else if (subcmd == STASH_SUBCMD_POP)
    rc = stash_pop(text_file, hunks);

  if (!rc) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

static void unknown_argument(char c);

static void
get_flags(int argc, char* argv[])
{
  while (true)
  {
    int c = getopt(argc, argv, ":hqv");
    if (c == -1) break;
    switch (c)
    {
      case 'h':
        help();
        exit(EXIT_SUCCESS);
        break;
      case 'q':
        stash_verbosity--;
        break;
      case 'v':
        stash_verbosity++;
        break;
      case '?':
        unknown_argument(optopt);
        break;
    }
  }
}

static void
unknown_argument(char c)
{
  printf("stash: unknown flag: '%c'\n", c);
  help();
  exit(EXIT_FAILURE);
}

#define NL "\n"

static char* help_string =
"stash: usage:" NL NL
"  stash push|pop <flags> <file> <hunks>?" NL NL
"  where hunks is" NL
"  * nothing -> interactive mode" NL
"  * a comma-separated list of integers" NL
"  * '@' -> all hunks" NL NL
"flags:" NL
"  -h : help" NL
"  -q : decrease verbosity (may be given several times)" NL
"  -v : increase verbosity (may be given several times)" NL
;

static void
help()
{
  printf("%s", help_string);
}
