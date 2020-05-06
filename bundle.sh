TARGET=./bundle/

cd www
npm install
npm run build
cd ..

python -m platformio run
python -m platformio run -t buildfs

mkdir -p ${TARGET}
cp ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/bin/bootloader_dio_40m.bin ${TARGET}/
cp ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin  ${TARGET}/
cp .pio/build/lolin_d32/partitions.bin ${TARGET}/
cp .pio/build/lolin_d32/firmware.bin ${TARGET}/
cp .pio/build/lolin_d32/spiffs.bin ${TARGET}/
