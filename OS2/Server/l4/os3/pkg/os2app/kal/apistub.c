/*
 *  API implementations
 *  parts (on the client side)
 *  These functions are those which are
 *  exported from the KAL.DLL virtual library.
 */

#include <l4/os3/gcc_os2def.h>
#include <l4/os3/apistub.h>

#include <dice/dice.h>

#include <l4/os3/os2server-client.h>
#include <l4/execsrv/os2exec-client.h>

#include <l4/l4rm/l4rm.h>
#include <l4/env/env.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dm_generic/consts.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/sys/kdebug.h>>

extern l4_threadid_t os2srv;
extern l4_threadid_t execsrv;

unsigned long
PvtLoadModule(char *pszName,
              unsigned long cbName,
              char const *pszModname,
              os2exec_module_t *s,
              unsigned long *phmod);

int
strlstlen(char *p)
{
  int l, len = 0;

  if (!p || !*p)
    return 2;
    
  while (*p) // skip all lines; break if NULL
  {
    /* skip one string */
    l = strlen(p) + 1;
    /* skip NULL character */
    p   += l;
    len += l;
  }

  if (!len)
    len++;
    
  len++; // include NULL symbol

  return len;
}


APIRET CDECL
KalRead (HFILE hFile, PVOID pBuffer,
            ULONG cbRead, PULONG pcbActual)
{
  CORBA_Environment env = dice_default_environment;
  APIRET  rc;

  LOG("started");
  
  if (!cbRead)
    return 0; /* NO_ERROR */
  
  rc = os2server_dos_Read_call(&os2srv, hFile, &pBuffer, &cbRead, &env);

  *pcbActual = cbRead;

  LOG("ended");
  return rc;
}


APIRET CDECL
KalWrite (HFILE hFile, PVOID pBuffer,
              ULONG cbWrite, PULONG pcbActual)
{
  CORBA_Environment env = dice_default_environment;
  APIRET  rc;

  LOG("started");
  LOG("hFile=%x", hFile);
  LOG("pBuffer=%x", pBuffer);
  LOG("cbWrite=%u", cbWrite);
  LOG("pcbActual=%x", pcbActual);

  if (!cbWrite)
    return 0; /* NO_ERROR */

  rc = os2server_dos_Write_call(&os2srv, hFile, pBuffer, &cbWrite, &env);

  *pcbActual = cbWrite;

  LOG("ended");
  return rc;
}


VOID CDECL
KalExit(ULONG action, ULONG result)
{
  CORBA_Environment env = dice_default_environment;

  // send OS/2 server a message that we want to terminate
  os2server_dos_Exit_send(&os2srv, action, result, &env);
  // wait until OS/2 server kills us
  l4_ipc_sleep(L4_IPC_NEVER);
}


APIRET CDECL
KalQueryCurrentDisk(PULONG pdisknum,
                        PULONG plogical)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_QueryCurrentDisk_call(&os2srv, pdisknum, plogical, &env);
}

APIRET CDECL
KalSetCurrentDir(PSZ pszDir)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_SetCurrentDir_call(&os2srv, pszDir, &env);
}

APIRET CDECL
KalSetDefaultDisk(ULONG disknum)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_SetDefaultDisk_call(&os2srv, disknum, &env);
}

APIRET CDECL
KalQueryCurrentDir(ULONG disknum,
                       PBYTE pBuf,
                       PULONG pcbBuf)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_QueryCurrentDir_call(&os2srv, disknum, &pBuf, pcbBuf, &env);
}


APIRET CDECL
KalQueryProcAddr(ULONG hmod,
                     ULONG ordinal,
                     const PSZ  pszName,
                     void  **ppfn)
{
  CORBA_Environment env = dice_default_environment;
  return os2exec_query_procaddr_call(&execsrv, hmod, ordinal,
                                     pszName, (void **)ppfn, &env);
}

/** attach dataspace to our address space. (any free address) */
int
attach_ds(l4dm_dataspace_t *ds, l4_uint32_t flags, l4_addr_t *addr)
{
  int error;
  l4_size_t size;
 
  if ((error = l4dm_mem_size(ds, &size)))
    {
      printf("Error %d (%s) getting size of dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }
  
  if ((error = l4rm_attach(ds, size, 0, flags, addr)))
    {
      printf("Error %d (%s) attaching dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }

  return 0;
}

/** attach dataspace to our address space. (concrete address) */
int
attach_ds_reg(l4dm_dataspace_t ds, l4_uint32_t flags, l4_addr_t addr)
{
  int error;
  l4_size_t size;
  l4_addr_t a = addr;

  /* get dataspace size */
  if ((error = l4dm_mem_size(&ds, &size)))
    {
      printf("Error %d (%s) getting size of dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }

  /* attach it to a given region */  
  if ((error = l4rm_attach_to_region(&ds, a, size, 0, flags)))
    {
      printf("Error %d (%s) attaching dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }

  return 0;
}

/*  Attaches all sections
 *  for a given module
 */
int
attach_module (ULONG hmod)
{
  CORBA_Environment env = dice_default_environment;
  l4exec_section_t sect;
  l4dm_dataspace_t ds, area_ds;
  l4_uint32_t flags;
  l4_addr_t   addr, map_addr;
  l4_size_t   map_size;
  l4_offs_t   offset;
  l4_threadid_t pager;
  unsigned type;
  int index;
  ULONG rc;

  index = 0; rc = 0;
  while (!os2exec_getsect_call (&execsrv, hmod, &index, &sect, &env) && !rc)
  {
    ds    = sect.ds; 
    addr  = sect.addr;
    type  = sect.info.type;
    flags = 0;  

    LOG("dataspace %x", ds); 
    LOG("addr %x", addr);
    
    if (type & L4_DSTYPE_READ)
      flags |= L4DM_READ;

    if (type & L4_DSTYPE_WRITE)
      flags |= L4DM_WRITE;

    if ((rc = l4rm_lookup(addr, &map_addr, &map_size,
                    &area_ds, &offset, &pager)) != L4RM_REGION_DATASPACE)
    {
      rc = attach_ds_reg (ds, flags, addr);
      if (!rc) 
        LOG("attached");
      else
      {
        LOG("attach_ds_reg returned %u", rc);
        break;
      }
    }
    else
    {
      LOG("map_addr=%x, map_size=%u, area_ds=%x",
          map_addr, map_size, area_ds);
      break;
    }
  }


  return 0; //rc;
}


/*  Attaches recursively a module and
 *  all its dependencies
 */
int
attach_all (ULONG hmod)
{
  CORBA_Environment env = dice_default_environment;
  ULONG imp_hmod, rc = 0;
  int index = 0;

  LOG("attach_all called");
  rc = attach_module(hmod);
  
  if (rc)
    return rc;
    
  while (!os2exec_getimp_call (&execsrv, hmod, &index, &imp_hmod, &env) && !rc)
  {
    if (!imp_hmod) // KAL fake module: no need to attach sections
      continue;
    //  return 0;
  
    rc = attach_all(imp_hmod);
  }

  LOG("attach_all returned: %u", rc);
  
  return rc;
}


unsigned long
PvtLoadModule(char *pszName,
              unsigned long cbName,
              char const *pszModname,
              os2exec_module_t *s,
              unsigned long *phmod)
{
  CORBA_Environment env = dice_default_environment;
  l4dm_dataspace_t ds = L4DM_INVALID_DATASPACE;
  ULONG hmod, rc;

  LOG("PvtLoadModule called");
  rc = os2exec_open_call (&execsrv, pszModname, &ds,
                          1, &hmod, &env);
  if (rc)
    return rc;

  *phmod = hmod;

  LOG("os2exec_open_call() called, rc=%d", rc);
  rc = os2exec_load_call (&execsrv, hmod, s, &env);

  if (rc)
    return rc;

  LOG("os2exec_load_call() called, rc=%d", rc);
  rc = os2exec_share_call (&execsrv, hmod, &env);

  if (rc)
    return rc;

  LOG("os2exec_share_call() called, rc=%d", rc);
  rc = attach_all(hmod);

  if (rc)
    return rc;

  LOG("attach_all() called, rc=%d", rc);

  return 0;
}


APIRET CDECL
KalLoadModule(PSZ pszName,
                  ULONG cbName,
                  char const *pszModname,
                  PULONG phmod)
{
  os2exec_module_t s;
  return PvtLoadModule(pszName, cbName, pszModname,
                       &s, phmod);
}

APIRET CDECL
KalExecPgm(char * pObjname,
               long cbObjname,
               unsigned long execFlag,
               char * pArg,
               char * pEnv,
               struct _RESULTCODES *pRes,
               char * pName)
{
  CORBA_Environment env = dice_default_environment;
  APIRET rc;
  int i, j;

  if (pArg == NULL)
  {
    pArg = "\0\0";
    i = 2;
  }
  else
    i = strlstlen(pArg);

  if (pEnv == NULL)
  {
    pEnv = "\0\0";
    j = 2;
  }
  else
    j = strlstlen(pEnv);
  
  LOG("started");
  LOG("pRes=%x", pRes);
  LOG("pObjname=%x",  pObjname);
  LOG("cbObjname=%x", cbObjname);
  LOG("len of pArg=%d", strlstlen(pArg));
  rc =  os2server_dos_ExecPgm_call (&os2srv, &pObjname,
                        &cbObjname, execFlag, pArg, i,
			pEnv, j,
                        pRes, pName, &env);
  LOG("ended");
  return rc;
}

APIRET CDECL
KalError(ULONG error)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_Error_call (&os2srv, error, &env);
}


//#define PAG_COMMIT   0x00000010
//#define OBJ_TILE     0x00000040

//#define PAG_EXECUTE  0x00000004
//#define PAG_READ     0x00000001
//#define PAG_WRITE    0x00000002
//#define PAG_GUARD    0x00000008

APIRET CDECL
KalAllocMem(PVOID *ppb,
            ULONG cb,
	    ULONG flags)
{
  l4_uint32_t rights = 0;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  int rc;

  if (flags & PAG_READ)
    rights |= L4DM_READ;

  if (flags & PAG_WRITE)
    rights |= L4DM_WRITE;

  /* Create a dataspace of a given size */
  rc = l4dm_mem_open(L4DM_DEFAULT_DSM, cb,
           4096, rights, "DosAllocMem dataspace", &ds);

  if (rc < 0)
    return 8; /* ERROR_NOT_ENOUGH_MEMORY */

  /* attach the created dataspace to our address space */
  rc = attach_ds(&ds, rights, &addr);

  if (rc)
    return 8; /* What to return? */
  
  *ppb = (void *)addr;
  
  return 0; /* NO_ERROR */
}

APIRET CDECL
KalFreeMem(PVOID pb)
{
  int rc;
  l4_addr_t addr;
  l4_size_t size;
  l4_offs_t offset;
  l4_threadid_t pager;
  l4dm_dataspace_t ds;
  
  rc = l4rm_lookup_region((l4_addr_t)pb, &addr, &size, &ds,
                     &offset, &pager);
  
  if (rc)
    return 487; /* ERROR_INVALID_ADDRESS */
    
  rc = l4rm_detach(addr);
  
  if (rc)
    return 5; /* ERROR_ACCESS_DENIED */
    
  rc = l4dm_close(&ds);
  
  if (rc)
    return 5; /* ERROR_ACCESS_DENIED */      
  
  return 0; /* NO_ERROR */
}

APIRET CDECL
KalResetBuffer(HFILE handle)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_ResetBuffer_call (&os2srv, handle, &env);
}

APIRET CDECL
KalSetFilePtr(HFILE handle,
              long ib,
	      ULONG method,
	      PULONG ibActual)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_SetFilePtr_call (&os2srv, handle, ib,
                                        method, ibActual, &env);
}

APIRET CDECL
KalClose(HFILE handle)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_Close_call (&os2srv, handle, &env);
}

APIRET CDECL
KalQueryHType(HFILE handle,
              PULONG pType,
	      PULONG pAttr)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_QueryHType_call (&os2srv, handle, pType, pAttr, &env);
}

APIRET CDECL
KalQueryDBCSEnv(ULONG cb,
                COUNTRYCODE *pcc,
		PBYTE pBuf)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_QueryDBCSEnv_call (&os2srv, &cb, pcc, &pBuf, &env);
}

APIRET CDECL
KalQueryCp(ULONG cb,
           PULONG arCP,
	   PULONG pcCP)
{
  CORBA_Environment env = dice_default_environment;
  return os2server_dos_QueryCp_call (&os2srv, cb, arCP, pcCP, &env);
}

APIRET CDECL
KalGetInfoBlocks(PTIB *pptib, PPIB *pppib)
{
  CORBA_Environment env = dice_default_environment;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_offs_t tib_offs, pib_offs;
  APIRET rc;

  /* get the dataspace with info blocks */
  rc = os2server_dos_GetInfoBlocks_call (&os2srv, &ds,
                           &tib_offs, &pib_offs, &env);

  if (rc)
    return rc;

  /* attach it */ 
  rc = attach_ds(&ds, L4DM_RW, &addr);
  if (rc)
  {
    LOG("error attaching ds!");
    return rc;
  }
  else
    LOG("ds attached");

  *pptib = (PTIB)((char *)addr + tib_offs);
  *pppib = (PPIB)((char *)addr + pib_offs);

  (*pptib)->tib_ptib2 = (PTIB2)((char *)(*pptib)->tib_ptib2 + addr);
  
  /* fixup fields */
  (*pppib)->pib_pchcmd = (char *)((*pppib)->pib_pchcmd) + addr;
  (*pppib)->pib_pchenv = (char *)((*pppib)->pib_pchenv) + addr;

  /* detach dataspace */
  rc = l4rm_detach(addr);
  
  if (rc)
  {
    LOG("error detaching ds!");
    return rc;
  }
  else
    LOG("ds detached");

  /* then attach it readonly */
  rc = attach_ds_reg(ds, L4DM_RO, addr);
  
  if (rc)
  {
    LOG("error attaching ds ro!");
    return rc;
  }
  else
    LOG("ds attached again ro");

  return 0; /* NO_ERROR */
}
