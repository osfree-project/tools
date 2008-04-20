/*
 *
 *
 */

#include <lip.h>
#include <types.h>
#include <loader.h>
#include <bpb.h>

#include <shared.h>

#include "fsys.h"
#include "fsd.h"
#include "struc.h"

extern unsigned long extended_memory;
#pragma aux extended_memory "*"

struct multiboot_info mbi;
unsigned long linux_text_len;
char *linux_data_real_addr;
char *linux_data_tmp_addr;

unsigned long relshift;

#pragma aux mbi "*"
#pragma aux extended_memory "*"
#pragma aux linux_data_real_addr "*"
#pragma aux linux_data_tmp_addr "*"
#pragma aux linux_text_len "*"

#pragma aux get_vbe_controller_info "*"
#pragma aux get_vbe_mode_info       "*"
#pragma aux set_vbe_mode            "*"
#pragma aux reset_vbe_mode          "*"
#pragma aux get_vbe_pmif            "*"

lip1_t *l1, lip1;
lip2_t *l2, lip2;

extern int __cdecl (*fsd_init)(lip1_t *l);

int i;
int  num_fsys = 0;

#ifndef STAGE1_5

#pragma aux  jmp_reloc "*"
void jmp_reloc(unsigned long addr);

#pragma aux  preldr_ds "*"
#pragma aux  preldr_es "*"
#pragma aux  preldr_ss_sp "*"

extern unsigned short preldr_ds;
extern unsigned short preldr_es;
extern unsigned long  preldr_ss_sp;

/* GDT */
#pragma aux gdt     "*"
#pragma aux gdtdesc "*"
extern struct desc gdt[5];
extern struct gdtr gdtdesc;

#pragma aux set_fsys    "*"
#pragma aux fsys_by_num "*"
int  set_fsys(char *fsname);
void fsys_by_num(int n, char *buf);

char linebuf[512];

// string table size;
#define STRTAB_LEN 0x300

// string table
char strtab[STRTAB_LEN];
// string table end position
int  strtab_pos = 0;

char lb[80];

char *fsys_list[FSYS_MAX];
char fsys_stbl[FSYS_MAX*10];

// max count of file aliases
#define MAX_ALIAS 0x10

/* Configuration got
   from .INI file      */
_Packed struct {
  char driveletter;
  char multiboot;
  char ignorecase;
  struct {
    char **fsys_list;
  } mufsd;
  struct {
    char name[128];
    int  base;
  } loader;
  struct {
    char name[128];
    int  base;
  } mini;
  struct {
    char *name;
    char *alias;
  } alias[MAX_ALIAS];
} conf = {0x80, 0, 0, {fsys_list}, {"/os2ldr", 0x10000},
          {"/os2boot", 0x7c0},};

char *preldr_path = "/boot/freeldr/"; // freeldr path
char *fsd_dir     = "fsd/";           // uFSD's subdir
char *cfg_file    = "preldr.ini";     // .INI file

#endif

#pragma aux lip1 "*"
#pragma aux lip2 "*"

extern mu_Open;
extern mu_Read;
extern mu_Close;
extern mu_Terminate;

extern unsigned long saved_drive;
extern unsigned long saved_partition;
extern unsigned long saved_slice;
extern unsigned long cdrom_drive;

extern FileTable ft;

extern unsigned short boot_flags;
extern unsigned long  boot_drive;
extern unsigned long  install_partition;

extern int mem_lower;
extern int mem_upper;
extern int fsmax;

void __cdecl real_test(void);
void __cdecl call_rm(fp_t);

#ifndef STAGE1_5

#pragma aux stop_floppy "*"
void stop_floppy(void);

/*   u_* functions are designed to be more like
 *   original IBM's microfsd functions, than GRUB
 *   functions and do not expose filepos
 *   and filemax variables to pre-loader
 *   clients, their values can be determined
 *   from these functions' return values
 */

/*   change/query current drive. all drives are
 *   specified in GRUB's notation, like (hdA,B)
 *   if successful, it returns 0, otherwise
 *   returns errnum.
 */

/*  open a file and return its size, return
 *  zero if no error, 1 otherwize
 */
unsigned int __cdecl
u_open (char *name, unsigned int *size)
{
  int rc;

  rc = freeldr_open(name);
  *size = 0;
 
  if (rc) {
    *size = filemax;
    return 0;
  }

  return 1;
}

/*  read count bytes to buf buffer and return
 *  the number of bytes actually read
 */
unsigned int __cdecl
u_read (char *buf, unsigned int count)
{
  return freeldr_read(buf, count);
}

/*  set or query current file position. If loffseek
 *  is equal to -1, the current offset is returned,
 *  if it's not -1, the specified offset is set
 */
unsigned int __cdecl
u_seek (int loffseek)
{
  if (loffseek == -1)
    return filepos;

  return freeldr_seek(loffseek);
}

/*  close current file
 */
void __cdecl
u_close (void)
{
  freeldr_close();
}

void __cdecl
u_terminate (void)
{
}

/*  32-bit disk low-level function.
 *  Read/write NSEC sectors starting from SECTOR in DRIVE disk with GEOMETRY
 *  from/into SEGMENT segment. If READ is BIOSDISK_READ, then read it,
 *  else if READ is BIOSDISK_WRITE, then write it. If an geometry error
 *  occurs, return BIOSDISK_ERROR_GEOMETRY, and if other error occurs, then
 *  return the error number. Otherwise, return 0.
 */
int __cdecl
u_diskctl (int func, int drive, struct geometry *geometry,
          int sector, int nsec, int addr)
{
  if (addr & 0xf != 0)      // check if addr is paragraph-aligned
    return ERR_UNALIGNED;

  if (func == BIOSDISK_READ || func == BIOSDISK_WRITE)
    return biosdisk(func, drive, geometry,
                    sector, nsec, addr >> 4);

  if (func == BIOSDISK_GEO)
    return get_diskinfo(drive, geometry);

  if (func == BIOSDISK_STOP_FLOPPY)
    stop_floppy();

  return 0;
}

/*
 *  Boot different types of
 *  operating systems.
 */

int __cdecl
u_boot (int type)
{
  if (type == KERNEL_TYPE_MULTIBOOT)
     return 0;

  if (type == KERNEL_TYPE_FREEBSD)
     return 0;

  if (type == KERNEL_TYPE_NETBSD)
     return 0;

  if (type == KERNEL_TYPE_LINUX)
     return 0;

  if (type == KERNEL_TYPE_BIG_LINUX)
     return 0;

  return 0;
}

/*   Load and relocate executable file
 */
int __cdecl
u_load (char *image, unsigned int size,
         char *load_addr, struct exe_params *p)
{
  return errnum;
}

/*   Changes/queries a parameter value.
 *   if action == ACT_GET, it returns value in 'val' variable;
 *   if action == ACT_SET, it changes its value to the value of 'val'.
 */

int __cdecl
u_parm (int parm, int action, unsigned int *val)
{
  switch (parm)
  {
    case PARM_BOOT_DRIVE:
      {
        if (action == ACT_GET)
          *val = boot_drive;
        else
          boot_drive = *val;

        return 0;
      };
    case PARM_CURRENT_DRIVE:
      {
        if (action == ACT_GET)
          *val = current_drive;
        else
          current_drive = *val;

        return 0;
      };
    case PARM_CURRENT_PARTITION:
      {
        if (action == ACT_GET)
          *val = current_partition;
        else
          current_partition = *val;

        return 0;
      };
    case PARM_CURRENT_SLICE:
      {
        if (action == ACT_GET)
          *val = current_slice;
        else
          current_slice = *val;

        return 0;
      };
    case PARM_SAVED_DRIVE:
      {
        if (action == ACT_GET)
          *val = saved_drive;
        else
          saved_drive = *val;

        return 0;
      };
    case PARM_SAVED_PARTITION:
      {
        if (action == ACT_GET)
          *val = saved_partition;
        else
          saved_partition = *val;

        return 0;
      };
    case PARM_SAVED_SLICE:
      {
        if (action == ACT_GET)
          *val = saved_slice;
        else
          saved_slice = *val;

        return 0;
      };
    case PARM_MBI:
      {
        if (action == ACT_GET)
        {
          *val = (unsigned int)(&mbi);
          return 0;
        }
        else
          return -1;
      };
    case PARM_ERRNUM:
      {
        if (action == ACT_GET)
          *val = errnum;
        else
          errnum = *val;

        return 0;
      };
    case PARM_FILEPOS:
      {
        if (action == ACT_GET)
          *val = filepos;
        else
          filepos = *val;

        return 0;
      };
    case PARM_FILEMAX:
      {
        if (action == ACT_GET)
          *val = filemax;
        else
          filemax = *val;

        return 0;
      };
    case PARM_EXTENDED_MEMORY:
      {
        if (action == ACT_GET)
          *val = extended_memory;
        else
          extended_memory = *val;

        return 0;
      };
    case PARM_LINUX_TEXT_LEN:
      {
        if (action == ACT_GET)
          *val = linux_text_len;
        else
          linux_text_len = *val;

        return 0;
      };
    case PARM_LINUX_DATA_REAL_ADDR:
      {
        if (action == ACT_GET)
          *val = (unsigned int)linux_data_real_addr;
        else
          linux_data_real_addr = (char *)*val;

        return 0;
      };
    case PARM_LINUX_DATA_TMP_ADDR:
      {
        if (action == ACT_GET)
          *val = (unsigned int)linux_data_tmp_addr;
        else
          linux_data_tmp_addr = (char *)*val;

        return 0;
      };
    default:
      ;
  }

  return -1;
}

void __cdecl
u_msg (char *s)
{
  char buf[0x100];

  grub_strncpy(buf, s, sizeof(buf));
  printmsg(buf);
}

void __cdecl
u_setlip (lip2_t *l)
{
  setlip2(l);
}

int __cdecl
u_vbectl(int func, int mode_number, void *info)
{
  struct pmif *pmif = (struct pmif *)info;
 
  switch (func)
  {
    case VBE_FUNC_RESET:
    {
      reset_vbe_mode ();
      return 0;
    }
    case VBE_FUNC_GET_CTRLR_INFO:
    {
      return get_vbe_controller_info ((struct vbe_controller *)info);
    }
    case VBE_FUNC_GET_MODE_INFO:
    {
      return get_vbe_mode_info (mode_number, (struct vbe_mode *)info);
    }
    case VBE_FUNC_SET_MODE:
    {
      return set_vbe_mode (mode_number);
    } 
    case VBE_FUNC_GET_PMIF:
    {
      get_vbe_pmif(pmif->pmif_segoff, pmif->pmif_len);
      return 0;
    }
    default:
    {
    }
  }
  return 1;
}

int open2 (char *filename)
{
  char buf[0x100];
  int  rc;

  for (fsys_type = 0; fsys_type < num_fsys; fsys_type++) 
  {
    fsys_by_num(fsys_type, buf);
    set_fsys(buf);
    rc = grub_open(filename);

    if (errnum == ERR_NONE)
      return rc;
  }

  return 1;
}

#endif

int
freeldr_open (char *filename)
{

#ifndef STAGE1_5

   char *p;
   int  i;
   int  i0 = 0;
   int  rc;
   char buf[0x100];

   u_msg(filename);
   u_msg("\r\n");

   /* prepend "/" to filename */
   if (*filename != '/' && *filename != '(') {
     buf[0] = '/';
     i0 = 1;
   }

   grub_strcpy(buf + i0, filename);

   for (i = 0; i < 128 && buf[i]; i++) {
     /* change "\" to "/" */
     if (buf[i] == '\\')
       buf[i] = '/';
   }

   /* try open filename as is */
   rc = grub_open(buf);

   if (conf.ignorecase) {
     /* if ignore case */
     if (!rc) {
       /* try to open uppercase filename */
       /* skip device name like "(cd)" */
       i = 0;
       if (*buf == '(')
         while (buf[i] && buf[i] != ')') i++;

       for (; i < 128 && buf[i]; i++) {
         buf[i] = grub_toupper(buf[i]);
       }

       rc = grub_open(buf);
     }

     if (!rc) {
       /* try to open lowercase filename */
       for (i = 0; i < 128 && buf[i]; i++)
         buf[i] = grub_tolower(buf[i]);

       rc = grub_open(buf);
     }
   }

   return rc;
#else
   return grub_open(filename);
#endif
}

int
freeldr_read (char *buf, int len)
{
   return grub_read(buf, len);
}

int
freeldr_seek (int offset)
{
   return grub_seek(offset);
}

void
freeldr_close (void)
{
   grub_close();
}

int  stage0_mount (void)
{
  return l1->lip_fs_mount();
}

int  stage0_read (char *buf, int len)
{
  return l1->lip_fs_read(buf, len);
}

int  stage0_dir (char *dirname)
{
  return l1->lip_fs_dir(dirname);
}

void stage0_close(void)
{
  if (l1->lip_fs_close)
    l1->lip_fs_close();
}

int  stage0_embed(int *start_sector, int needed_sectors)
{
  if (l1->lip_fs_embed)
    return l1->lip_fs_embed(start_sector, needed_sectors);

  return 0;
}

#ifndef STAGE1_5

void setlip2(lip2_t *l2)
{
  l2->u_lip2magic       = LIP2_MAGIC;
  l2->u_open            = &u_open;
  l2->u_read            = &u_read;
  l2->u_seek            = &u_seek;
  l2->u_close           = &u_close;
  l2->u_terminate       = &u_terminate;
  l2->u_load            = &u_load;
  l2->u_boot            = &u_boot;
  l2->u_parm            = &u_parm;
  l2->u_diskctl         = &u_diskctl;
  l2->u_vbectl          = &u_vbectl;
  l2->u_msg             = &u_msg;
  l2->u_setlip          = &u_setlip;
}

#endif

void setlip1(lip1_t *l1)
{
  l1->lip_open  = &freeldr_open;
  l1->lip_read  = &freeldr_read;
  l1->lip_seek  = &freeldr_seek;
  l1->lip_close = &freeldr_close;
  l1->lip_term  = 0;

  //l->lip_memcheck = 0; //&grub_memcheck;
  l1->lip_memset   = &grub_memset;
  l1->lip_memmove  = &grub_memmove;
  l1->lip_strcpy   = 0; //&grub_strcpy;
  l1->lip_strcmp   = 0; //&grub_strcmp;
  l1->lip_memcmp   = &grub_memcmp;
  l1->lip_strlen   = 0; //&grub_strlen;
  l1->lip_isspace  = &grub_isspace;
  l1->lip_tolower  = &grub_tolower;

  l1->lip_substring = &substring;
  //l1->lip_pos       = 0;
  //l1->lip_clear     = 0;

  l1->lip_devread   = &devread;
  l1->lip_rawread   = &rawread;

  l1->lip_mem_lower = &mem_lower;
  l1->lip_mem_upper = &mem_upper;

  l1->lip_filepos   = &filepos;
  l1->lip_filemax   = &filemax;

  l1->lip_buf_drive = &buf_drive;
  l1->lip_buf_track = &buf_track;
  l1->lip_buf_geom  = &buf_geom;

  l1->lip_errnum    = &errnum;

  l1->lip_saved_drive = &saved_drive;
  l1->lip_saved_partition = &saved_partition;

  l1->lip_current_drive = &current_drive;
  l1->lip_current_partition = &current_partition;
  l1->lip_current_slice = &current_slice;
  l1->lip_part_start    = &part_start;
  l1->lip_part_length   = &part_length;
  l1->lip_fsmax         = &fsmax;
#ifndef STAGE1_5
  l1->lip_printmsg      = &printmsg;
  l1->lip_printb        = &printb;
  l1->lip_printw        = &printw;
  l1->lip_printd        = &printd;
#endif
}

void setlip(void)
{
  l1 = &lip1; 
  setlip1(l1);
#ifndef STAGE1_5
  l2 = &lip2;
  setlip2(l2);
#endif
}

#ifndef STAGE1_5

/* if s is a string of type "var = val" then this
 * function returns var
 */
char *var(char *s)
{
  int i;

  i = grub_index('=', s);
  if (i) grub_strncpy(lb, s, i - 1);
  lb[i - 1] = '\0';

  return lb;
}

/* if s is a string of type "var = val" then this
 * function returns val
 */
char *val(char *s)
{
  int i, l;

  i = grub_index('=', s);
  l = grub_strlen(s) - i;
  if (i) grub_strncpy(lb, s + i, l);
  lb[l] = '\0';

  return lb;
}

/*  Strip leading and trailing
 *  spaces
 */
char *strip(char *s)
{
  char *p = s;
  int  i;

  i = grub_strlen(p) - 1;
  while (grub_isspace(p[i])) p[i--] = '\0'; // strip trailing spaces
  while (grub_isspace(*p)) p++;             // strip leading spaces

  return p;
}

/*  Add a string to the string table
 *  and return its address
 */
char *
stradd(char *s)
{
  int  k;
  char *l;

  k = grub_strlen(s);

  if (strtab_pos + k < STRTAB_LEN) {
    l = strtab + strtab_pos;
    grub_strcpy(l, s);
    l[k] = '\0';
    strtab_pos += k + 1;
  } else {
    /* no space in buffer */
    return 0;
  }

  return l;
}

/*  Returns a next line from a file in memory
 *  and changes current position (*p)
 */
char *getline(char **p)
{
  int  i = 0;
  char *q = *p;
  char *s;

  if (!q)
    panic("getline(): zero pointer: ", "*p");

  while (*q != '\n' && *q != '\r' && *q != '\0' && i < 512)
    linebuf[i++] = *q++;

  if (*q == '\r') q++;
  if (*q == '\n') q++;

  linebuf[i] = '\0';
  *p = q;

  s = strip(linebuf);

  if (!*s)
    return s;

  /* skip comments */
  i = grub_index(';', s);
  if (i) s[i - 1] = '\0';

  s = strip(s);

  /* if empty line, get new line */
  if (!*s)
    return getline(p);

  return s;
}


/*  Parse .INI file
 *
 */
int parse_cfg(void)
{
  char buf[512];
  int  k, l, i, size, rc;
  char *cfg, *p, *line, *s, *r;

  l = grub_strlen(preldr_path);
  grub_memmove(buf, preldr_path, l);
  grub_memmove(buf + l, cfg_file, grub_strlen(cfg_file));
  i = grub_strlen(cfg_file);
  buf[l + i] = '\0';

  rc = freeldr_open(buf);

  cfg = (char *)(EXT2BUF_BASE);

  if (rc) {
    printmsg("file ");
    printmsg(buf);
    printmsg(" opened, ");
    size = freeldr_read((void *)cfg, -1);
    printmsg("size: ");
    printd(size);
    printmsg("\r\n");
  } else
    return 0;

  /* parse .INI file */
  p = cfg;
  while (*p) {
    line = strip(getline(&p));
    if (!*line) continue;

    if (!grub_strcmp(strip(line), "[global]")) {
      /* [global] section */
      while (*p) {
        line = getline(&p);
        if (!*line) break;

        if (!grub_strcmp(strip(var(line)), "driveletter")) {
          grub_strcpy(buf, strip(val(line)));
          conf.driveletter = *buf; // first letter
          conf.driveletter = grub_tolower(conf.driveletter) - 'a';
          if (conf.driveletter > 1) // not floppy
            conf.driveletter = conf.driveletter - 2 + 0x80;

          continue;
        }

        if (!grub_strcmp(strip(var(line)), "multiboot")) {
          if (!grub_strcmp(strip(val(line)), "yes") ||
              !grub_strcmp(strip(val(line)), "on")) {
            conf.multiboot = 1;
          } else {
            conf.multiboot = 0;
          }

          continue;
        }

        if (!grub_strcmp(strip(var(line)), "ignorecase")) {
          if (!grub_strcmp(strip(val(line)), "yes") ||
              !grub_strcmp(strip(val(line)), "on")) {
            conf.ignorecase = 1;
          } else {
            conf.ignorecase = 0;
          }

          continue;
        }
      }
      continue;
    }
    if (!grub_strcmp(strip(line), "[microfsd]")) {
      /* [microfsd] section */
      while (*p) {
        line = getline(&p);
        if (!*line) break;

        if (!grub_strcmp(strip(var(line)), "list")) {
          grub_strcpy(fsys_stbl, strip(val(line)));
          s = fsys_stbl;
          r = fsys_stbl;
          num_fsys = 0;
          while (*s) {
            while (*s && *s != ',') s++;
            *s = '\0';
            fsys_list[num_fsys] = r;
            r = s + 1;
            s = r;
            num_fsys++;
          }

          continue;
        }
      }
      continue;
    }
    if (!grub_strcmp(strip(line), "[loader]")) {
      /* [loader] section */
      while (*p) {
        line = getline(&p);
        if (!*line) break;

        if (!grub_strcmp(strip(var(line)), "name")) {
          grub_strcpy(conf.loader.name, strip(val(line)));

          continue;
        }
        if (!grub_strcmp(strip(var(line)), "base")) {
          conf.loader.base = grub_aton(strip(val(line)));

          continue;
        }
      }
      continue;
    }
    if (!grub_strcmp(strip(line), "[minifsd]")) {
      /* [loader] section */
      while (*p) {
        line = getline(&p);
        if (!*line) break;

        if (!grub_strcmp(strip(var(line)), "name")) {
          grub_strcpy(conf.mini.name, strip(val(line)));
          if (!grub_strcmp(conf.mini.name, "none"))
            *(conf.mini.name) = '\0';

          continue;
        }
        if (!grub_strcmp(strip(var(line)), "base")) {
          conf.mini.base = grub_aton(strip(val(line)));

          continue;
        }
      }
      continue;
    }
    if (!grub_strcmp(strip(line), "[aliases]")) {
      /* file aliases section */
      char *q;
      i = 0;

      while (*p) {
        line = getline(&p);
        if (!*line) break;

        // printmsg(line); printmsg("\r\n");

        if (!grub_strcmp(strip(var(line)), "enable")) {
          continue;
        }

        q = stradd(strip(var(line)));
        if (!q) panic("too many aliases, no space in buffer!", "");

        conf.alias[i].name  = q;
        // printmsg(q); printmsg(" ");

        q = stradd(strip(val(line)));
        if (!q) panic("too many aliases, no space in buffer!", "");

        conf.alias[i].alias = q;
        // printmsg(q); printmsg("\r\n");

        i++;
      }
      continue;
    }
  }

  return 1;
}

void panic(char *msg, char *file)
{
  printmsg("\r\nFatal error: \r\n");
  printmsg(msg);
  printmsg(file);

  __asm {
    cli
    hlt
  }
}

/*  Relocate a file in memory using its
 *  .rel file.
 *  base is a file base, rel_file is .rel
 *  file name and shift is the relocation
 *  shift
 */
void reloc(char *base, char *rel_file, unsigned long shift)
{
  int  i, n, rc;
  char buf[0x1000];

  typedef _Packed struct {
    unsigned short addr;
    unsigned char  shift;
  } rel_item;

  rel_item *p;
  unsigned long *addr;

  /* Load .rel file */
  rc = freeldr_open(rel_file);

  if (rc) {
    // buf = (char *)(EXT2BUF_BASE);
    rc  = freeldr_read(buf, -1);
  } else {
    panic("Can't open .rel file:", rel_file);
  }

  /* number of reloc items */
  n = *((unsigned short *)(buf)) / 3;
  p = (rel_item *)(buf + 2);

  for (i = 0; i < n; i++) {
    addr  = (unsigned long *)(base + p[i].addr);
    *addr += shift >> p[i].shift;
  }
}

#endif

int init(void)
{
  int rc, files;
  char *buf;
  char *fn;
  char *s;
  unsigned long ldrlen = 0, mfslen = 0;
  unsigned long ldrbase;
  bios_parameters_block *bpb;
  struct geometry geom;
  unsigned short *p;
  unsigned long  *q;
  struct desc *z;
  unsigned long base;
  int i, k;

  /* Set boot drive and partition.  */
  saved_drive = boot_drive;
  saved_partition = install_partition;

  /* Set cdrom drive.   */
  /* Get the geometry.  */
  if (get_diskinfo (boot_drive, &geom)
      || ! (geom.flags & BIOSDISK_FLAG_CDROM))
    cdrom_drive = GRUB_INVALID_DRIVE;
  else
    cdrom_drive = boot_drive;

  /* setting LIP */
  setlip();

  grub_memmove((void *)(UFSD_BASE), (void *)(EXT_BUF_BASE), EXT_LEN);

  /* call uFSD init (set linkage) */
  fsd_init = (void *)(EXT_BUF_BASE); // uFSD base address
  fsd_init(l1);

#ifndef STAGE1_5

  /* parse config file */
  if (!parse_cfg())
    panic("config file doesn't exist!\r\n", "preldr.ini");

  /* Show config params */
  printmsg("\r\nConfig parameters:");
  printmsg("\r\ndriveletter = "); printb(conf.driveletter);
  printmsg("\r\nmultiboot = ");   printb(conf.multiboot);
  printmsg("\r\nignorecase = ");   printb(conf.ignorecase);
  printmsg("\r\nFilesys: ");
  for (i = 0; i < FSYS_MAX && conf.mufsd.fsys_list[i]; i++) {
    printmsg(conf.mufsd.fsys_list[i]);
    printmsg(" ");
  }
  printmsg("\r\nloader.name = "); printmsg(conf.loader.name);
  printmsg("\r\nloader.base = "); printd(conf.loader.base);
  printmsg("\r\nmini.name = "); printmsg(conf.mini.name);
  printmsg("\r\nmini.base = "); printd(conf.mini.base);
  printmsg("\r\n");
  for (i = 0; i < MAX_ALIAS; i++) {
    s = conf.alias[i].name;
    if (!s) break;
    printmsg(s);
    printmsg("=");
    s = conf.alias[i].alias;
    printmsg(s);
    printmsg("\r\n");
  }

  /* load os2ldr */
  fn = conf.loader.name;
  rc = freeldr_open(fn);
  printmsg("freeldr_open(\"");
  printmsg(fn);
  printmsg("\") returned: ");
  printd(rc);

  buf = (char *)(conf.loader.base);

  if (rc) {
    ldrlen = freeldr_read(buf, -1);

    printmsg("\r\nfreeldr_read() returned size: ");
    printd(ldrlen);
    printmsg("\r\n");
  } else {
    panic("Can't open loader file: ", fn);
  }

  /* load minifsd */
  fn = conf.mini.name;
  if (*fn) { // is minifsd needed?
    rc = freeldr_open(fn);
    printmsg("freeldr_open(\"");
    printmsg(fn);
    printmsg("\") returned: ");
    printd(rc);

    buf = (char *)(conf.mini.base);

    if (rc) {
      mfslen = freeldr_read(buf, -1);

      printmsg("\r\nfreeldr_read() returned size: ");
      printd(mfslen);
      printmsg("\r\n");
    } else {
      panic("Can't open minifsd filename: ", fn);
    }
  }

  printmsg("mem_lower=");
  printd(mem_lower);

  /* calculate highest available address
     -- os2ldr base or top of low memory  */

  k = ldrlen >> (PAGESHIFT - 3);
  i = k >> 3;

  /* special case: os2ldr of aurora sized
     44544 bytes, 44544 >> 12 == 0xa      */
  if (k == 0x57) i++; // one page more

  if (!conf.multiboot) // os2ldr
    ldrbase = ((mem_lower >> (PAGESHIFT - KSHIFT)) - (i + 3)) << PAGESHIFT;
  else                 // multiboot loader
    ldrbase =  mem_lower << KSHIFT;

  printmsg("\r\n");
  printd(ldrbase);

  /* the correction shift added while relocating */
  relshift = ldrbase - (PREFERRED_BASE + 0x10000);

  /* move preldr and uFSD */
  grub_memmove((char *)(ldrbase - 0x10000),
               (char *)(EXT_BUF_BASE + EXT_LEN - 0x10000),
               0x10000);

  __asm {
    mov  eax, relshift
    /* switch stack to the place of relocation */
    add  esp,   eax
    add  ebp,   eax
    /* fixup words in stack */
    add  [ebp], eax
    add  [ebp - 0xc],  eax
    add  [ebp + 0x18], eax
    add  [ebp + 0x20], eax
  }

  printmsg("\r\nrelshift=");
  printd(relshift);
  printmsg("\r\n");

  /* fixup preldr and uFSD */
  reloc((char *)(STAGE0_BASE  + relshift), "\\boot\\freeldr\\preldr0.rel", relshift);
  reloc((char *)(EXT_BUF_BASE + relshift), "\\boot\\freeldr\\fsd\\iso9660.rel", relshift);

  /* jump to relocated pre-loader */
  jmp_reloc(relshift);
  /* now we are at the place of relocation */

  //l1 += relshift;
  //l2 += relshift;

  /* setting LIP */
  setlip();

  /* save boot drive uFSD */
  grub_memmove((void *)(UFSD_BASE), (void *)(EXT_BUF_BASE), EXT_LEN);
  /* call uFSD init (set linkage) */
  fsd_init = (void *)(EXT_BUF_BASE); // uFSD base address
  fsd_init(l1);

  z = &gdt;

  /* fix bases of gdt descriptors */
  base = STAGE0_BASE;
  for (i =  3; i < 5; i++) {
    (z + i)->ds_baselo  = base & 0xffff;
    (z + i)->ds_basehi1 = (base & 0xff0000) >> 16;
    (z + i)->ds_basehi2 = (base & 0xff000000) >> 24;
  }

  /* set new gdt */
  __asm {
    lea  eax, gdtdesc
    lgdt fword ptr [eax]
  }

  /* set boot flags */
  boot_flags = BOOTFLAG_MICROFSD;

  files = 2;
  if (*fn) { // if minifsd present
    files++;
    boot_flags |=  BOOTFLAG_MINIFSD;
  }

  /* set filetable */
  ft.ft_cfiles = files;
  ft.ft_ldrseg = conf.loader.base >> 4;
  ft.ft_ldrlen = ldrlen; // 0x3800;
  ft.ft_museg  = (EXT_BUF_BASE + EXT_LEN - 0x10000) >> 4; // 0x8500; -- OS/2 2.0       // 0x8600; -- OS/2 2.1
                                         // 0x8400; -- Merlin & Warp3 // 0x8100; -- Aurora
  ft.ft_mulen  = 0x10000;     // It is empirically found maximal value
  ft.ft_mfsseg = conf.mini.base >> 4;    // 0x7c;
  ft.ft_mfslen = mfslen;  // 0x95f0;
  ft.ft_ripseg = 0; // 0x800;   // end of mfs
  ft.ft_riplen = 0; // 62*1024 - mfslen; //0x3084;  // max == 62k - mfslen

  ft.ft_muOpen.seg       = STAGE0_BASE >> 4;
  ft.ft_muOpen.off       = (unsigned short)(&mu_Open);

  ft.ft_muRead.seg       = STAGE0_BASE >> 4;
  ft.ft_muRead.off       = (unsigned short)(&mu_Read);

  ft.ft_muClose.seg      = STAGE0_BASE >> 4;
  ft.ft_muClose.off      = (unsigned short)(&mu_Close);

  ft.ft_muTerminate.seg  = STAGE0_BASE >> 4;
  ft.ft_muTerminate.off  = (unsigned short)(&mu_Terminate);

  if (boot_drive == cdrom_drive) { // booting from CDROM drive
    /* set BPB */
    bpb = (bios_parameters_block *)(BOOTSEC_BASE + 0xb);
    // fill fake BPB
    grub_memset((void *)bpb, 0, sizeof(bios_parameters_block));

    bpb->sect_size  = 0x800;
    bpb->clus_size  = 0x40;
    bpb->n_sect_ext = geom.total_sectors; // 0x30d;
    bpb->media_desc = 0xf8;
    bpb->track_size = 0x3f;
    bpb->heads_cnt  = 0xff;
    bpb->disk_num   = (unsigned char)(boot_drive & 0xff);
    bpb->log_drive  = conf.driveletter; // 0x92;
    bpb->marker     = 0x29;
  }

  /* Init info in mbi structure */
  init_bios_info();

  if (conf.multiboot) {
    /* return to loader from protected mode */
    unsigned long ldr_base = conf.loader.base;

    __asm {
      mov  dx, boot_flags
      mov  dl, byte ptr boot_drive

      // edi == pointer to filetable
      lea  edi, ft

      // esi ==  pointer to BPB
      mov  esi, BOOTSEC_BASE + 0xb

      // magic in eax
      mov  eax, BOOT_MAGIC

      // ebx == pointer to LIP
      mov  ebx, l2

      push ldr_base
      retn
    }
  }

#else


#endif

  return 0;
}

#pragma aux init     "_*"
#pragma aux real_test "*"
#pragma aux call_rm   "*"
#pragma aux printmsg  "*"
#pragma aux printb    "*"
#pragma aux printw    "*"
#pragma aux printd    "*"