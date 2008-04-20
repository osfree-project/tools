/*  Set environment variables 
 */

'@echo off'

parse arg cfg
parse source os addr src

if cfg = '' 
  then call usage

/* Build the full path to the config file */
if pos('/', translate(cfg, '/', '\')) == 0 then do
   l = lastpos('/', translate(src, '/', '\'))
   if l > 0 then cfg = delstr(src, l + 1) || cfg
end

if stream(cfg, 'c', 'query exist') = '' 
  then signal usage

/* Read config file */
rc = stream(cfg, 'c', 'open read')

if rc \= 'READY:' then do
  say 'Can''t open file ' || cfg || '!'
  exit 1
end

do while lines(cfg) > 0
  line = linein(cfg)
  /* strip comments */
  p = pos(';', line)
  if p > 0 then line = delstr(line, p)
  line = strip(line)
  if line = '' then iterate
  interpret line
end

call stream cfg, 'c', 'close'

env = 'ENVIRONMENT'

if pos(':', imgdir) == 2 then 
  imgdir = delstr(imgdir, 1, 2)
imgdir1 = translate(imgdir, '/', '\')

if os == 'OS/2' then do
  wosdir = '\binp'
end; else
if os == 'WINDOWS' | os == 'WINNT' then do
    wosdir = '\binnt'
end; else
if os == 'UNIX' | os == 'LINUX' then do
  wosdir = '\binl'
end; else do
  say 'Unsupported OS!'
  exit 2
end

tools = root || '\tools\bin'
path  = value('PATH',, env)
path  = watcom || wosdir || ';' || tools || ';' || tkpath || '\bin;' || fppath || ';' || path
include = watcom || '\h;' || watcom || '\h\dos;' || watcom || '\h\os2'
finclude = watcom || '\src\fortran'
edpath = watcom || '\eddat'

if os == 'OS/2' then do
  help = value('HELP',, env)
  help = watcom || wosdir || '\help;' || help
  bookshelf = value('BOOKSHELF',, env)
  bookshelf = watcom || wosdir || '\help;' || bookshelf
  beginlibpath = watcom || wosdir || '\dll'
  libos2 = watcom || '\lib386\os2'
end; else do
  help = ''; bookshelf = '';
  beginlibpath = ''; libos2 = '';
end

lib = watcom || '\lib286;' || watcom || '\lib286\dos;' || root || '\lib'

vars = 'WATCOM ROOT IMGDIR IMGDIR1 TOOLS PATH INCLUDE ',
       'FINCLUDE EDPATH HELP BOOKSHELF BEGINLIBPATH ',
       'LIBOS2 LIB OS SHELL REXX'

/* Set vars */
do i = 1 to words(vars)
  var = word(vars, i)
  val = value(var)
  call value var, val, env
end

if os == 'OS/2' then 'set beginlibpath=' || beginlibpath


exit 0
/* ------------------------------------ */
usage:

say 'Usage:'
say
say 'setenv <config file>'
say

/* ------------------------------------ */