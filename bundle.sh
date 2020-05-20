# Build Web UI
cd www
npm install
npm run build
cd ..

# Build for all boards
python -m platformio run
python -m platformio run -t buildfs

# Package
for TARGET in binaries/*/
do
    BOARD=`basename ${TARGET}`

    echo "Packaging ${BOARD} ..."

    cp ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/bin/bootloader_dio_40m.bin ${TARGET}/
    cp ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin  ${TARGET}/
    cp .pio/build/${BOARD}/partitions.bin ${TARGET}/
    cp .pio/build/${BOARD}/firmware.bin ${TARGET}/
    cp .pio/build/${BOARD}/spiffs.bin ${TARGET}/
done

zip binaries.zip binaries