| Kit Per Bank: ev_settings handler and SETTINGS row data.
| kpb_row_select, kpb_row_draw and kpb_row_change are defined in main.c.
| Firmware addresses: 1.53 by default; OS154 selects 1.54's.

        .ifdef  OS154
        .equ    .LF_STR_CTOR, 0x4017af20
        .else
        .equ    .LF_STR_CTOR, 0x4017ac20
        .endif

        .text

| ev_settings handler: add the KIT PER BANK row to the SETTINGS menu.
        .globl  kpb_settings
kpb_settings:
        pea     kpb_row
        move.l  8(%sp), -(%sp)          | the menu
        jsr     core_additem
        addq.l  #8, %sp
        rts

| label(): builds the row's std::string in a0 (d0 returns a0, as the MenuItem expects).
        .balign 2
kpb_row_label:
        link    %a6, #-4
        move.l  %d2, -(%sp)
        pea     -1(%a6)
        pea     kpb_row_str
        move.l  %a0, %d2
        move.l  %a0, -(%sp)
        jsr     .LF_STR_CTOR
        lea     12(%sp), %sp
        move.l  %d2, %d0
        move.l  -8(%a6), %d2
        unlk    %a6
        rts

        .balign 4
kpb_row:
        .long   kpb_row_label, kpb_row_select, kpb_row_draw, kpb_row_change
kpb_row_str:
        .asciz  "KIT PER BANK"
        .balign 2
