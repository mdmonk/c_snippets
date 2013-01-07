/*
 * This will simply give information on the ELF bin, and with variables
 * given as arguments to the program (assuming they are located in data
 * segment), give the offsets between the variables (useful in exploiting
 * overflows in data segment).  Warning: this can give false info
 *
 * To compile: gcc -o datatest datatest.c -lbfd -liberty
 * -- Matt Conover (Shok)
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <bfd.h>

#define ERROR -1

/* default data types needed */
bfd *abfd;
bfd_format bfdformat; /* contains format */
enum bfd_flavour bfdflavour; /* contains the file flavour */

asection *bssSect, *heapSect, *pltSect;


void setup(); /* does the main bfd setup */
void showStats(); /* show stats on file */

void dumpsects(asection *sect);
void getSyms(char *sym1, char *sym2);

void bfderror(char *fmt, ...);

int main(int argc, char **argv)
{
   if (argc <= 1)
   {
      fprintf(stderr, "Usage: %s <filename> <var1> <var2>\n\n", argv[0]);
      fprintf(stderr, "Where var1/var2 are variables in the program\n");
      fprintf(stderr, "E.g.: %s ./%s main abfd\n", argv[0], argv[0]);

      exit(ERROR); 
   }

   bfd_init();
   bfd_set_error_handler((bfd_error_handler_type)bfderror);

   abfd = bfd_openr(argv[1], NULL);

   if (abfd == NULL)
   {
      bfderror("[bfd_openr] error with opening %s", argv[1]);
      exit(ERROR);
   }

   setup(), showStats();

   printf("\nHit enter to continue..."), (void)getchar();

   dumpsects(bfd_get_section_by_name(abfd, ".text"));
   dumpsects(bfd_get_section_by_name(abfd, ".plt"));

   dumpsects(bfd_get_section_by_name(abfd, ".data"));
   dumpsects(bfd_get_section_by_name(abfd, ".bss"));

   getSyms(argv[2], argv[3]);

   bfd_close(abfd);
   return 0;
}

/* ------------------------------------- */

void setup()
{
   bfdformat = bfd_object;
   if (bfd_check_format(abfd, bfdformat) == false)
   {
      (void)fprintf(stderr, "error: only object files are supported\n\n");
      exit(ERROR);
   }

   bfdflavour = bfd_get_flavour(abfd);
   if (bfdflavour == ERROR)
   {
      bfderror("[bfd_get_flavour] error");
      exit(ERROR);
   }

   /* ---------------------------------------------------- */

   if ((bfdflavour == bfd_target_unknown_flavour) ||
       ((bfdflavour != bfd_target_aout_flavour) &&
        (bfdflavour != bfd_target_coff_flavour) &&
        (bfdflavour != bfd_target_elf_flavour) &&
        (bfdflavour != bfd_target_msdos_flavour)))
   {
      (void)fprintf(stderr, "Only current supported formats (flavours): "
                            "ELF, COFF, AOUT, and MS-DOS\n");

      exit(ERROR);
   }
}

/* ------------------------------------- */

void showStats()
{   
   (void)printf("Filename: %s\n\n", bfd_get_filename(abfd));
   (void)printf("File's target: %s\n", bfd_get_target(abfd));

   (void)printf("File's endianess: ");

   if (bfd_little_endian(abfd)) (void)printf("little endian\n");
   else (void)printf("big endian\n");
   
   (void)printf("Bits per byte on file's arch: %u bits\n",
                bfd_arch_bits_per_byte(abfd));

   (void)printf("Bits per address on file's arch: %u bits\n\n",
                bfd_arch_bits_per_address(abfd));

   (void)printf("Start address: %p\n", 
                (void *)bfd_get_start_address(abfd));

}

/* ------------------------------------- */

/* Dump all the executable's section (independent of type) */
void dumpsects(asection *argsect)
{
   if (argsect == NULL) 
   {
      fprintf(stderr, "\nargsect = NULL... returning\n");
      return;
   }

   printf("\n-------------------------------------------------\n\n");
   printf("Section: %s\n", argsect->name);

   if (((u_long)argsect->vma - (u_long)argsect->lma) != 0)
     printf("Section Virtual Memory Address (VMA): %p\n",
            (u_long *)argsect->vma);

   printf("Section Load Memory Address (LMA): %p\n", 
          (u_long *)argsect->lma);

   /* ----------------------------------------- */

   printf("Important section flags: ");

   if (argsect->flags & SEC_ALLOC) printf("ALLOC ");
   if (argsect->flags & SEC_LOAD) printf("LOAD ");
   if (argsect->flags & SEC_READONLY) printf("READONLY ");
   if (argsect->flags & SEC_CODE) printf("CODE ");
   if (argsect->flags & SEC_DATA) printf("DATA ");

   putchar('\n');
}

/* ------------------------------------- */

void getSyms(char *sym1, char *sym2)
{
   int sym1addr = 0, sym2addr = 0;
   char foundsym1 = 0, foundsym2 = 0;

   asymbol **symtab;
   long i, storage, numSyms;

   if ((sym1 == NULL) || (sym2 == NULL))
   {
      fprintf(stderr, "\nsym1 or sym2 == NULL.. returning\n");
      return;
   }

   storage = bfd_get_symtab_upper_bound(abfd);
   if (storage == ERROR)
   {
      bfderror("[bfd_get_symtab_upper_bound] error");
      exit(ERROR);
   }

   else if (storage == 0)
   {
      (void)fprintf(stderr, "No storage was needed (???)\n");
      return;
   }

   /* -------------------------------------- */

   symtab = (asymbol **)malloc(storage);

   numSyms = bfd_canonicalize_symtab(abfd, symtab);
   if (numSyms == ERROR)
   {
      bfderror("[bfd_canonicalize_symtab] error");
      exit(ERROR);
   }

   for (i = 0; i < numSyms; i++)
   {
      if (isalpha(symtab[i]->name[0]))
      {
         if (strcmp(symtab[i]->name, sym1) == 0)
         {
            foundsym1 = 1; 
            sym1addr = symtab[i]->value;

            printf("-------------------------------------------------\n");

            printf("Symbol's name: %s\n", symtab[i]->name);
            printf("Symbol's value: 0x%x (%d)\n",
                   (u_int)symtab[i]->value, (u_int)symtab[i]->value);

            printf("Symbol's address: %p\n", symtab[i]->udata.p);
            printf("Symbol's flags: "), fflush(stdout);

            if (symtab[i]->flags & BSF_LOCAL) (void)printf("LOCAL ");
            else if (symtab[i]->flags & BSF_LOCAL) (void)printf("GLOBAL ");

            if (symtab[i]->flags & BSF_EXPORT) (void)printf("EXPORT ");

            if (symtab[i]->flags & BSF_FUNCTION) (void)printf("FUNCTION ");
            if (symtab[i]->flags & BSF_WEAK) (void)printf("WEAK ");

            if (symtab[i]->flags & BSF_SECTION_SYM)
               (void)printf("SECTION_SYM ");

            if (symtab[i]->flags & BSF_FILE) (void)printf("FILE ");
            if (symtab[i]->flags & BSF_DYNAMIC) (void)printf("DYNAMIC ");
            if (symtab[i]->flags & BSF_OBJECT) (void)printf("OBJECT ");

            putchar('\n');
         }

         if (strcmp(symtab[i]->name, sym2) == 0)
         {
            foundsym2 = 1;
            sym2addr = symtab[i]->value;

            printf("\n-------------------------------------------------\n");
            (void)printf("Symbol's name: %s\n", symtab[i]->name);
            (void)printf("Symbol's value: 0x%x (%d)\n",
                         (u_int)symtab[i]->value, (u_int)symtab[i]->value);

            (void)printf("Symbol's address: %p\n", symtab[i]->udata.p);
            (void)printf("Symbol's flags: "), (void)fflush(stdout);

            if (symtab[i]->flags & BSF_LOCAL) (void)printf("LOCAL ");
            else if (symtab[i]->flags & BSF_LOCAL) (void)printf("GLOBAL ");

            if (symtab[i]->flags & BSF_EXPORT) (void)printf("EXPORT ");

            if (symtab[i]->flags & BSF_FUNCTION) (void)printf("FUNCTION ");
            if (symtab[i]->flags & BSF_WEAK) (void)printf("WEAK ");

            if (symtab[i]->flags & BSF_SECTION_SYM)
               (void)printf("SECTION_SYM ");

            if (symtab[i]->flags & BSF_FILE) (void)printf("FILE ");
            if (symtab[i]->flags & BSF_DYNAMIC) (void)printf("DYNAMIC ");
            if (symtab[i]->flags & BSF_OBJECT) (void)printf("OBJECT ");

            printf("\n\n");
         }
      }
   }

   if ((sym1addr > sym2addr) && ((sym1addr > 0) && (sym2addr > 0)))
   {
      (void)fprintf(stderr, "error: symbol '%s' > symbol '%s' "
                    "(can't overflow %s into %s)\n", 
                    sym1, sym2, sym1, sym2);

      return;
   }

   else if ((sym1addr == sym2addr) && (sym1addr > 0) && (sym2addr > 0))
      (void)printf("%s's addr = %s's addr\n", sym1, sym2);

   else if ((sym1addr > 0) && (sym2addr > 0))
      (void)printf("offset from %s to %s = 0x%x (%d) bytes\n", sym1, sym2,
                   sym2 - sym1, sym2 - sym1);

   if (foundsym1 != 1) 
      fprintf(stderr, "error: couldn't find symbol '%s'\n", sym1);

   if (foundsym2 != 1) 
      fprintf(stderr, "error: couldn't find symbol '%s'\n", sym2);

   free(symtab);
}

/* ------------------------------------- */

/* our error message handler */
void bfderror(char *fmt, ...)
{
   va_list va;
   char errbuf[512] = {0};

   va_start(va, fmt);
   vsnprintf(errbuf, sizeof(errbuf)-1, fmt, va);
   va_end(va);

   bfd_perror(errbuf);
}


