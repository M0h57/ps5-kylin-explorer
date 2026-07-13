.section .rodata
.global kylin_embedded_font_start
.global kylin_embedded_font_end
.align 16
kylin_embedded_font_start:
.incbin "assets/fonts/NotoSansSC-Regular.ttf"
kylin_embedded_font_end:
.byte 0
