/* Main program for interactive testing.  For maximum output, compile
   this and regex.c with -DDEBUG.  */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "regex.h"

/* Don't bother to guess about <string.h> vs <strings.h>, etc.  */
extern int strlen ();

#define BYTEWIDTH 8

// extern void printchar ();
// extern char upcase[];

// - upcase.c
//
/* Indexed by a character, gives the upper case equivalent of the
   character.  */
unsigned char upcase[0400] = 
  { 000, 001, 002, 003, 004, 005, 006, 007,
    010, 011, 012, 013, 014, 015, 016, 017,
    020, 021, 022, 023, 024, 025, 026, 027,
    030, 031, 032, 033, 034, 035, 036, 037,
    040, 041, 042, 043, 044, 045, 046, 047,
    050, 051, 052, 053, 054, 055, 056, 057,
    060, 061, 062, 063, 064, 065, 066, 067,
    070, 071, 072, 073, 074, 075, 076, 077,
    0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
    0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
    0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
    0140, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
    0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
    0130, 0131, 0132, 0173, 0174, 0175, 0176, 0177,
    0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
    0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
    0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
    0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
    0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
    0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
    0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
    0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
    0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
    0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
    0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
    0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
    0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
    0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
    0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
    0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
  };

static void scanstring ();
static void print_regs ();

int
main (argc, argv)
     int argc;
     char **argv;
{
  int i;
  struct re_pattern_buffer buf;
  char fastmap[(1 << BYTEWIDTH)];

  /* Allow a command argument to specify the style of syntax.  You can
     use the `syntax' program to decode integer syntax values.  */
  if (argc > 1)
    re_set_syntax (atoi (argv[1]));

  buf.allocated = 0;
  buf.buffer = NULL;
  buf.fastmap = fastmap;
  buf.translate = upcase;

  for (;;)
    {
      char pat[500], str[500];
      struct re_registers regs;

      /* Some C compilers don't like `char pat[500] = ""'.  */
      pat[0] = 0;
      
      printf ("Pattern (%s) = ", pat);
      gets (pat);
      scanstring (pat);

      if (feof (stdin))
        {
          putchar ('\n');
          exit (0);
	}

      if (*pat)
	{
          re_compile_pattern (pat, strlen (pat), &buf);
	  re_compile_fastmap (&buf);
#ifdef DEBUG
	  print_compiled_pattern (&buf);
#endif
	}

      printf ("String = ");
      gets (str);	/* Now read the string to match against */
      scanstring (str);

      i = re_match (&buf, str, strlen (str), 0, &regs);
      printf ("Match value  %d.\t", i);
      if (i >= 0)
        print_regs (regs);
      putchar ('\n');
      
      i = re_search (&buf, str, strlen (str), 0, strlen (str), &regs);
      printf ("Search value %d.\t", i);
      if (i >= 0)
        print_regs (regs);
      putchar ('\n');
    }

  /* We never get here, but what the heck.  */
  return 0;
}

void
scanstring (s)
     char *s;
{
  char *write = s;

  while (*s != '\0')
    {
      if (*s == '\\')
	{
	  s++;

	  switch (*s)
	    {
	    case '\0':
	      break;

	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      *write = *s++ - '0';

	      if ('0' <= *s && *s <= '9')
		{
		  *write = (*write << 3) + (*s++ - '0');
		  if ('0' <= *s && *s <= '9')
		    *write = (*write << 3) + (*s++ - '0');
		}
	      write++;
	      break;

	    case 'n':
	      *write++ = '\n';
	      s++;
	      break;

	    case 't':
	      *write++ = '\t';
	      s++;
	      break;

	    default:
	      *write++ = *s++;
	      break;
	    }
	}
      else
	*write++ = *s++;
    }

  *write++ = '\0';
}

/* Print REGS in human-readable form.  */

void
print_regs (regs)
     struct re_registers regs;
{
  int i, end;

  printf ("Registers: ");
  
  if (regs.num_regs == 0 || regs.start[0] == -1)
    {
      printf ("(none)");
    }
  else
    {
      /* Find the last register pair that matched.  */
      for (end = regs.num_regs - 1; end >= 0; end--)
        if (regs.start[end] != -1)
          break;

      printf ("[%d ", regs.start[0]);
      for (i = 1; i <= end; i++)
        printf ("(%d %d) ", regs.start[i], regs.end[i]);
      printf ("%d]", regs.end[0]);
    }
}

// - printchar.c
//
void
printchar (c)
     char c;
{
  if (c < 040 || c >= 0177)
    {
      putchar ('\\');
      putchar (((c >> 6) & 3) + '0');
      putchar (((c >> 3) & 7) + '0');
      putchar ((c & 7) + '0');
    }
  else
    putchar (c);
}

