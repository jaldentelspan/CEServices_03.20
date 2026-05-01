#!/bin/bash
# Script to build iES or VNS switch 
# must run from [VNS SRC]/build

test_for_failure() {
  if [ $1 -ne 0 ] ; then
    echo "System Failed during the following exicution:: $current_task"
    exit;
  fi
}
print_out_preparded_list() {
  echo "**** The following items are being prepaid ****"
  echo $switch_device;
  echo $reference_device;
  echo $flash_device;
  echo
}
validate_menu_entry() {
while true; do
print_out_preparded_list
read -p "Would you like to continue [y on n]?" validate_menu_entry
case $validate_menu_entry in
  [yY]) return 0; break;;
  [nN]) return -1; break;;
    * ) echo "Please enter a valid selection.";
esac
done
}
main_menu() {
while true; do
  echo "Enter a menu selection:"
  echo "1: Make iES-6"
  echo "2: Make iES-12"
  echo "0: exit"
  read -p "Please enter a value?" menu_item
  case $menu_item in
    1) switch_device="iES-6"; reference_device="vns_6_ref.mk"; flash_device="flash_ies6"; flash_name="ies_6_flash_";break;;
    2) switch_device=iES-12; reference_device=vns_12_ref.mk; flash_device=flash_ies12; flash_name="ies_12_flash_";break;;
    0) exit;;
    * ) echo "Please enter a valid selection.";
  esac

done
}
copy_resulting_files() {
NEW_DIR_NAME="$(date '+%d-%b-%Y')"
SRC1="$HOME/project/vns/Software/VNS_3_20/CEServices/src/build/obj/$switch_device.*"
SRC2="$HOME/project/vns/Software/VNS_3_20/CEServices/src/build/tools/$flash_name*"
NEW_DIR="$HOME/project/vns/Software/Builds/$switch_device/$NEW_DIR_NAME"
WRK_DIR="$HOME/project/vns/Software/Builds/$switch_device"
echo $NEW_DIR
cd $WRK_DIR
mkdir $NEW_DIR_NAME
cp $SRC1 $NEW_DIR
cp $SRC2 $NEW_DIR

}

main_menu;
print_out_preparded_list

: << 'END'
# make errors out if a config.mk does not exsist.  one is created to satisfy make.
ln -s configs/$reference_device config.mk
if [ $? -eq 0 ] ; then
  echo "Reference file config.mk did not exists.  Creating one to make mrproper happy..."
else
  echo "Reference file config.mk currently exists as expected"
fi


current_task="setting environment....."
echo $current_task
source ./set_environment.sh

current_task="Making mrproper....."
echo $current_task
make mrproper
test_for_failure $? 

current_task= "Creating link....."
echo $current_task
ln -s configs/$reference_device config.mk
test_for_failure $? 

current_task= "Making redboot....."
echo $current_task
make redboot
test_for_failure $? 

current_task= "Making ....."
echo $current_task
make
test_for_failure $? 

current_task="Coping to TFTP....."
echo $current_task
sudo cp ./obj/$switch_device.dat /var/lib/tftpboot/$switch_device.dat
test_for_failure $? 

current_task="Setting file attributes....."
echo $current_task
sudo chmod 777 /var/lib/tftpboot/$switch_device.dat
test_for_failure $? 


current_task="Moving to ./tools....."
echo $current_task
cd tools
test_for_failure $? 

current_task="Making flash....."
echo $current_task
make $flash_device
test_for_failure $? 


current_task="returning to ../build....."
echo $current_task
cd ..
test_for_failure $? 


current_task="coping resulting files....."
echo $current_task
copy_resulting_files
test_for_failure $? 


current_task="....."

END


current_task="Coping to TFTP....."
echo $current_task
sudo cp ./obj/$switch_device.dat /var/lib/tftpboot/$switch_device.dat
test_for_failure $? 

