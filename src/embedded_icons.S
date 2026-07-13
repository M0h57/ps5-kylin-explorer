.section .rodata
.align 16

.macro EMBED_ICON symbol, path
.global \symbol\()_start
.global \symbol\()_end
\symbol\()_start:
.incbin "\path"
\symbol\()_end:
.byte 0
.align 16
.endm

EMBED_ICON kylin_icon_button_cross, assets/misc/icon-button-cross.rgba
EMBED_ICON kylin_icon_button_circle, assets/misc/icon-button-circle.rgba
EMBED_ICON kylin_icon_button_square, assets/misc/icon-button-square.rgba
EMBED_ICON kylin_icon_button_triangle, assets/misc/icon-button-triangle.rgba
EMBED_ICON kylin_icon_button_options, assets/misc/icon-button-options.rgba
EMBED_ICON kylin_icon_button_l1, assets/misc/icon-button-l1.rgba
EMBED_ICON kylin_icon_button_l2, assets/misc/icon-button-l2.rgba
EMBED_ICON kylin_icon_button_r1, assets/misc/icon-button-r1.rgba
EMBED_ICON kylin_icon_button_r2, assets/misc/icon-button-r2.rgba
EMBED_ICON kylin_icon_button_touchpad, assets/misc/icon-button-touchpad.rgba
EMBED_ICON kylin_icon_button_up_down, assets/misc/icon-button-up-down.rgba
EMBED_ICON kylin_icon_button_left_right, assets/misc/icon-button-left-right.rgba
EMBED_ICON kylin_icon_button_l3, assets/misc/icon-button-l3.rgba
EMBED_ICON kylin_icon_button_r3, assets/misc/icon-button-r3.rgba
EMBED_ICON kylin_icon_button_rs, assets/misc/icon-button-rs.rgba

EMBED_ICON kylin_icon_menu_refresh, assets/misc/icon-menu-refresh.rgba
EMBED_ICON kylin_icon_menu_sort, assets/misc/icon-menu-sort.rgba
EMBED_ICON kylin_icon_menu_properties, assets/misc/icon-menu-properties.rgba
EMBED_ICON kylin_icon_menu_new_folder, assets/misc/icon-menu-new-folder.rgba
EMBED_ICON kylin_icon_menu_rename, assets/misc/icon-menu-rename.rgba
EMBED_ICON kylin_icon_menu_copy, assets/misc/icon-menu-copy.rgba
EMBED_ICON kylin_icon_menu_cut, assets/misc/icon-menu-cut.rgba
EMBED_ICON kylin_icon_menu_forward, assets/misc/icon-menu-forward.rgba
EMBED_ICON kylin_icon_menu_delete, assets/misc/icon-menu-delete.rgba

EMBED_ICON kylin_icon_explorer_folder, assets/misc/icon-explorer-folder.rgba
EMBED_ICON kylin_icon_explorer_file, assets/misc/icon-explorer-file.rgba

EMBED_ICON kylin_icon_settings_language, assets/misc/icon-settings-language.rgba
EMBED_ICON kylin_icon_settings_temperature, assets/misc/icon-settings-temperature.rgba
EMBED_ICON kylin_icon_settings_installed_apps, assets/misc/icon-settings-installed-apps.rgba
EMBED_ICON kylin_icon_settings_operation_log, assets/misc/icon-settings-operation-log.rgba

EMBED_ICON kylin_icon_common_in_operation, assets/misc/icon-common-in-operation.rgba
EMBED_ICON kylin_icon_common_success, assets/misc/icon-common-success.rgba
EMBED_ICON kylin_icon_common_error, assets/misc/icon-common-error.rgba
EMBED_ICON kylin_icon_common_package_installing, assets/misc/icon-common-package-installing.rgba

EMBED_ICON kylin_icon_common_usb_0, assets/misc/icon-common-usb-0.rgba
EMBED_ICON kylin_icon_common_usb_1, assets/misc/icon-common-usb-1.rgba
EMBED_ICON kylin_icon_common_usb_2, assets/misc/icon-common-usb-2.rgba
EMBED_ICON kylin_icon_common_usb_3, assets/misc/icon-common-usb-3.rgba
EMBED_ICON kylin_icon_common_usb_4, assets/misc/icon-common-usb-4.rgba
EMBED_ICON kylin_icon_common_usb_5, assets/misc/icon-common-usb-5.rgba
EMBED_ICON kylin_icon_common_usb_6, assets/misc/icon-common-usb-6.rgba
EMBED_ICON kylin_icon_common_ext_1, assets/misc/icon-common-ext-1.rgba
