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
  echo "0: exit"
  echo "1: Make iES-6"
  echo "2: Make iES-12"
  echo "3: Make EPE iES-12"
  echo "4: Make Mitr FPGA 12"
  echo "5: Make iES-6 IRIG input"
  echo "6: Make iES-12 IRIG input"
  echo "7: Make iES-6 GVCP"
  echo "8: Make iES-12 GVCP"
  echo "9: Make iES-12 2.0 "
  read -p "Please enter a value?" menu_item
  case $menu_item in
    0) exit;;
    1) switch_device="iES-6"; reference_device="vns_6_ref.mk"; flash_device="flash_ies6"; flash_name="ies_6_flash_";break;;
    2) switch_device="iES-12"; reference_device="vns_12_ref.mk"; flash_device="flash_ies12"; flash_name="ies_12_flash_";break;;
    # 3) switch_device="iES-12"; reference_device="vns_12rev5_ref.mk"; flash_device="flash_ies12"; flash_name="ies_12_flash_";break;;
    # 3) switch_device="iES-12"; reference_device="vns_12rev5_ref.mk"; flash_device="flash_ies12rev5"; flash_name="ies_12rev5_flash_";break;;
    3) switch_device="iES-12"; reference_device="vns_12rev5_ref.mk"; flash_device="flash_epe"; flash_name="epe_flash_";break;;
    4) switch_device="MITR_SWITCH"; reference_device="mitr_switch_ref.mk"; flash_device="flash_ies12"; flash_name="ies_12_flash_";break;;
    5) switch_device="iES-6"; reference_device="vns_6_irig_ref.mk"; flash_device="flash_ies6"; flash_name="ies_6_flash_";break;;
    6) switch_device="iES-12"; reference_device="vns_12_irig_ref.mk"; flash_device="flash_ies12"; flash_name="ies_12_flash_";break;;
    7) switch_device="iES-6"; reference_device="vns_6_gvcp_ref.mk"; flash_device="flash_ies6"; flash_name="ies_6_flash_";break;;
    8) switch_device="iES-12"; reference_device="vns_12_gvcp_ref.mk"; flash_device="flash_ies12"; flash_name="ies_12_flash_";break;;
    9)  switch_device="iES-12_2_0";    reference_device="ies_2_0_ref.mk";      flash_device="flash_ies_2_0";flash_name="ies_2_0_flash_";       EPE=1;GVCP=0;break;;
    * ) echo "Please enter a valid selection.";
  esac

done
}
copy_resulting_files() {
NEW_DIR_NAME="$(date '+%d-%b-%Y')"
#SRC1="$HOME/TelspanDataProjectsInternal/iES_12/Development/Software/VNS_3_20/CEServices/src/build/obj/$switch_device.*"
#SRC2="$HOME/TelspanDataProjectsInternal/iES_12/Development/Software/VNS_3_20/CEServices/src/build/tools/$flash_name*"
#NEW_DIR="$HOME/TelspanDataProjectsInternal/iES_12/Development/Software/Builds/$switch_device/$NEW_DIR_NAME"
#WRK_DIR="$HOME/TelspanDataProjectsInternal/iES_12/Development/Software/Builds/$switch_device"
# SRC1="../../VNS_3_20/CEServices/src/build/obj/$switch_device.*"
# SRC2="../../VNS_3_20/CEServices/src/build/tools/$flash_name*"
# NEW_DIR="./$NEW_DIR_NAME"
# WRK_DIR="../../../../../Builds/$switch_device"
WRK_DIR="../../../../Builds/$switch_device/$NEW_DIR_NAME"

SRC1="$PWD/obj/$switch_device.*"
SRC2="$PWD/tools/$flash_name*"
mkdir -p "$WRK_DIR"
echo $NEW_DIR
# cd $WRK_DIR
# mkdir $NEW_DIR_NAME
# cp -f $SRC1 $NEW_DIR
# cp -f $SRC2 $NEW_DIR
cp -f $SRC1 $WRK_DIR
cp -f $SRC2 $WRK_DIR
# echo "$PWD/$NEW_DIR_NAME"
echo "$WRK_DIR"

}

main_menu;
print_out_preparded_list

#: << 'END'
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
make redboot -j80
test_for_failure $? 

current_task= "Making ....."
echo $current_task
make -j80
test_for_failure $? 

#current_task="Coping to TFTP....."
#echo $current_task
#sudo cp ./obj/$switch_device.dat /var/lib/tftpboot/$switch_device.dat
#test_for_failure $? 

#current_task="Setting file attributes....."
#echo $current_task
#sudo chmod 777 /var/lib/tftpboot/$switch_device.dat
#test_for_failure $? 


current_task="Moving to ./tools....."
echo $current_task
cd tools
test_for_failure $? 

current_task="Making flash....."
echo $current_task
make $flash_device
test_for_failure $? 

#END



current_task="returning to ../build....."
echo $current_task
cd ..
test_for_failure $? 

current_task="coping resulting files....."
echo $current_task
copy_resulting_files
test_for_failure $? 

current_task="....."




