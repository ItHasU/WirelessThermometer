BOARD=$1

PREVIOUS_DIR=`pwd`
cd $BOARD
python -m esptool $(< args.cmd)
cd ${PREVIOUS_DIR}
