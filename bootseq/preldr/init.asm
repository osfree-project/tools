;
; init.asm
; stage0 real-mode startup
;

name init

public  boot_flags
public  boot_drive
public  install_partition
public  ft

public  stage0_init
public  force_lba

public  _text16_begin
extrn   _text16_end  :dword
extrn   bss_end      :dword
extrn   preldr0_end  :dword

extrn   gdt          :byte
extrn   init         :near
extrn   call_pm      :near
extrn   lip          :dword

ifndef STAGE1_5

extrn   mu_Open      :far
extrn   mu_Read      :far
extrn   mu_Close     :far
extrn   mu_Terminate :far

extrn   printhex8    :near

extrn   preldr_ds    :word
extrn   preldr_ss_sp :dword
extrn   preldr_es    :word

endif

include fsd.inc
include struc.inc
include loader.inc
include bpb.inc

.386p

_TEXT16 segment byte public 'CODE' use16

_text16_begin:

;
; stage0 startup
;
stage0_init:
        jmp  short real_start
;
; Configuration variables
;

; variables block size
CONF_VARS_SIZE equ  8

; this variable is filled with uFSD size,
; if stage0 is loaded concatenated with
; uFSD and needs to be relocated to proper
; addresses, otherwise it must be zero.
uFSD_size     dw    0
stage0_size   dw    0
force_lba     db    0

padsize  equ  CONF_VARS_SIZE - ($ - stage0_init - 2)
pad           db    padsize dup (0)

real_start:
        ; Set segment registers
        ; to CS value, set stack
        ; to the end of this segment
        mov  ax, cs
        mov  ds, ax

        cli

        mov  ss, ax
        mov  sp, 0ffffh
        mov  bp, sp

        sti

        ; Save physical boot drive
        mov  eax, offset _TEXT:boot_drive - STAGE0_BASE
        mov  [eax], dl

        cld
        mov  cx, uFSD_size
        jcxz skip_reloc  ; if uFSD_size == 0, it means that
                         ; uFSD and stage0 are already loaded
                         ; to proper places by the bootsector.

        ; relocate uFSD to EXT_BUF_BASE
        shr  cx, 1
        inc  cx

        ; preldr0 size = uFSD begin
        mov  si, stage0_size

        push si

        mov  eax, EXT_BUF_BASE
        shr  eax, 4
        mov  es,  ax
        xor  di,  di

        rep  movsw

        ; relocate itself to STAGE0_BASE
        pop  cx          ; stage0 length

        push cx

        shr  cx, 1
        inc  cx

        xor  si, si

        mov  eax, STAGE0_BASE
        shr  eax, 4
        mov  es, ax
        xor  di, di

        rep  movsw
skip_reloc:
        ; clear BSS
        mov  ecx, offset _TEXT:bss_end
        mov  eax, offset _TEXT:preldr0_end
        sub  ecx, eax    ; BSS length
        mov  ebx, STAGE0_BASE
        sub  eax, ebx
        shr  ebx, 4
        mov  es, bx
        mov  edi, eax
        xor  ax, ax

        rep  stosb

        ; relocate boot sector to safe place
        push ds
        push es

        mov  cx, 100h
        xor  ax, ax
        mov  ds, ax
        mov  si, 7C00h
        mov  di, ax
        mov  eax, BOOTSEC_BASE
        shr  eax, 4
        mov  es, ax

        rep  movsw

        pop  es
        pop  ds

        ; jump to the new location
        push es
        push reloc
        retf
reloc:
        ; Set segment registers
        mov  ax, cs
        mov  ds, ax
        mov  es, ax

        cli

        mov  ss, ax
        mov  sp, STAGE0_LEN

        sti

        ; set 16-bit segment (_TEXT16) base
        ; in GDT for protected mode
        mov  eax, STAGE0_BASE
        mov  esi, offset _TEXT:gdt - STAGE0_BASE
        mov  [si][3*8].ds_baselo, ax
        mov  [si][4*8].ds_baselo, ax
        ror  eax, 16
        mov  [si][3*8].ds_basehi1, al
        mov  [si][4*8].ds_basehi1, al
        ror  eax, 8
        mov  [si][3*8].ds_basehi2, al
        mov  [si][4*8].ds_basehi2, al

        ; call 32-bit protected mode init
        mov  eax, offset _TEXT:init
        push eax
        call call_pm
        add  sp, 4

ifndef STAGE1_5
        ; save pre-loader segment registers and stack
        mov  ax, ds
        mov  preldr_ds, ax
        mov  ax, es
        mov  preldr_es, ax
        mov  ax, sp
        mov  word ptr preldr_ss_sp, ax
        mov  ax, ss
        mov  word ptr preldr_ss_sp + 2, ax

        ;
        ; pass structures to os2ldr
        ;
        mov  eax, offset _TEXT:boot_flags - STAGE0_BASE
        ; set bootflags
        mov  dx, [eax]

        ; set bootdrive
        mov  eax, offset _TEXT:boot_drive - STAGE0_BASE
        mov  dl, [eax]

        mov  edi, offset _TEXT:ft - STAGE0_BASE

        ; set BPB
        mov  eax, BOOTSEC_BASE
        shr  eax, 4
        mov  ds,  ax
        mov  si,  0bh           ; 3 + 8 = 11 -- BPB offset from the beginning of boot sector

        ; set LIP pointer and magic
        mov  ebx, offset _TEXT:lip
        mov  eax, BOOT_MAGIC

        ; return to os2ldr
        push LDR_SEG
        push 0
        retf

else

; Control transferring from lite version to full one

endif

;fname   db "/boot/bootblock",0
;fsize   dd 0

_TEXT16 ends

_TEXT   segment byte public 'CODE' use32

boot_flags         dw 0
boot_drive         dd 0
install_partition  dd 0ffffffh

ft                 FileTable <>

_TEXT   ends

        end stage0_init