set(ASSET_DIR "assets")
idf_component_register(SRCS "main.c" "vga.c" "pie.S" "gfx.c" "comp.c" "input.c" "serial.c" "perf.c" "remote.c"
                            "wm.c" "ui.c" "menu.c" "shell.c" "dlg.c" "apps.c" "apps_basic.c"
                            "storage.c" "app_loader.c" "app_files.c" "app_editor.c" "app_misc.c" "net.c" "app_net.c" "app_browser.c"
                            "diag.c" "usb_input.c" "${ASSET_DIR}/assets_gen.c"
                       INCLUDE_DIRS "." "${ASSET_DIR}"
                       REQUIRES esp_driver_pcnt esp_driver_gpio esp_driver_uart esp_psram spi_flash
                                esp_timer esp_hw_support fatfs wear_levelling vfs xtensa mbedtls esp_app_format usb esp_wifi esp_netif esp_event nvs_flash esp_http_client esp-tls
                       EMBED_FILES "../disk/Documents/Welcome.txt" "../disk/Pictures/Bevel.bmp"
                                   "../disk/Apps/Snake.elf" "../disk/Apps/Hello.elf" "../disk/Apps/CrashTest.elf" "../disk/Apps/Doom.elf")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
