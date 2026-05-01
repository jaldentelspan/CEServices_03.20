#!/bin/bash
# Script to build iES or VNS switch 
# must run from [VNS SRC]/build

export VNS_VERSION_MAJOR="$(date +"%Y%m%d")"
# export VNS_VERSION_MINOR="$(date +"%H%M%S")"
export VNS_VERSION_MINOR="$(date +"%k%M%S")"
export CODE_REVISION="$VNS_VERSION_MAJOR$VNS_VERSION_MINOR"
export VNS_VERSION_COMMON_IP=2
export VNS_VERSION_PCKG_ID=1
#define VNS_VERSION_COMMON_IP	2
#define VNS_VERSION_PCKG_ID	1

test_for_failure() {
  if [ $1 -ne 0 ] ; then
    echo "System Failed during the following exicution:: $current_task"
    exit $1;
  fi
}
print_out_preparded_list() {
  echo "**** The following items are being prepaid ****"
  echo "**************************************************"
  echo "Creating IP.Package Version : $VNS_VERSION_COMMON_IP.$VNS_VERSION_PCKG_ID"
  echo "Creating Version number: $VNS_VERSION_MAJOR.$VNS_VERSION_MINOR"
  echo "**************************************************"
  echo $switch_device;
  echo $reference_device;
  echo $flash_device;
  echo
  read -p "Please enter a value?" menu_item
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
echo "Current User: $USER"
validate_menu_entry

echo "$1" 
while true; do
if [ $1 -gt 0 -a $1 -lt 16 ] ; then
    menu_item="$1"
    echo "$1" 
else
      echo "Enter a menu selection:"
      echo " 0:  exit"
      echo " 1:  Make iES-6"
      echo " 2:  Make iES-6 with GVCP"
      echo " 3:  Make iES-6 with IRIG input"
      echo " 4:  Make iES-12"
      echo " 5:  Make iES-12 with GVCP"
      echo " 6:  Make iES-12 with IRIG/EPE"
      echo " 7:  Make NCGU"
      echo " 8:  Make IGU"
      echo " 9:  Make MITR"
      echo "10:  Make ESIR"
      echo "11:  Make iES-16"
      echo "12:  Make iES-8"
      echo "13:  Make VESM"
      echo "14:  Make ies-12-2-0"
      echo "15:  Make ies-16-2-0"
      read -p "Please enter a value?" menu_item
fi
  case $menu_item in
    1)   echo "case 1"; switch_device="iES-6";        reference_device="vns_6_ref.mk";        flash_device="flash_ies6";   flash_name="ies_6_flash_";         EPE=0;GVCP=0;break;;
    2)   echo "case 2"; switch_device="iES-6";        reference_device="vns_6_gvcp_ref.mk";   flash_device="flash_ies6";   flash_name="ies_6_flash_";         EPE=0;GVCP=1;break;;
    3)   echo "case 3"; switch_device="iES-6";        reference_device="vns_6_irig_ref.mk";   flash_device="flash_ies6";   flash_name="ies_6_flash_";         EPE=0;GVCP=0;break;;
    4)   echo "case 4"; switch_device="iES-12";       reference_device="vns_12_ref.mk";       flash_device="flash_ies12";  flash_name="ies_12_flash_";        EPE=0;GVCP=0;break;;
    5)   echo "case 5"; switch_device="iES-12";       reference_device="vns_12_gvcp_ref.mk";  flash_device="flash_ies12";  flash_name="ies_12_flash_";        EPE=0;GVCP=1;break;;
    6)   echo "case 6"; switch_device="iES-12";       reference_device="vns_12_irig_ref.mk";  flash_device="flash_epe";    flash_name="epe_flash_";           EPE=1;GVCP=0;break;;
    7)   echo "case 7"; switch_device="NCGU_SWITCH";  reference_device="ncgu_switch_ref.mk";  flash_device="flash_ncgu";   flash_name="ncgu_flash";           EPE=0;GVCP=0;break;;
    8)   echo "case 8"; switch_device="IGU_SWITCH";   reference_device="igu_switch_ref.mk";   flash_device="flash_igu";    flash_name="igu_flash";            EPE=0;GVCP=0;break;;
    9)   echo "case 9"; switch_device="MITR_SWITCH";  reference_device="mitr_switch_ref.mk";  flash_device="flash_mitr";   flash_name="mitr_flash";           EPE=0;GVCP=0;break;;
    10)  echo "case 10"; switch_device="eSIR";         reference_device="esir_switch_ref.mk";  flash_device="flash_esir";   flash_name="esir_flash";           EPE=0;GVCP=0;break;;
    11)  echo "case 11"; switch_device="iES-16";       reference_device="vns_16_ref.mk";       flash_device="flash_epe";    flash_name="epe_flash_";           EPE=1;GVCP=0;break;;
    12)  echo "case 12"; switch_device="iES-8";        reference_device="vns_8_ref.mk";        flash_device="flash_ies8";   flash_name="ies_8_flash_";         EPE=0;GVCP=0;break;;
    13)  echo "case 13"; switch_device="VESM_SWITCH";  reference_device="vesm_switch_ref.mk";  flash_device="flash_vesm";   flash_name="vesm_flash";           EPE=0;GVCP=0;break;;
    # 14)  echo "case 14"; switch_device="iES-12-20";    reference_device="ies_2_0_ref.mk";      flash_device="flash_ies_2_0";flash_name="ies_2_0_flash_";       EPE=1;GVCP=0;break;;
    14)  echo "case 14"; switch_device="iES-12_2_0";    reference_device="ies_2_0_ref.mk";      flash_device="flash_ies_2_0";flash_name="ies_2_0_flash_";       EPE=1;GVCP=0;break;;
    15)  echo "case 15"; switch_device="iES-16_2_0";    reference_device="ies_16_2_0_ref.mk";      flash_device="flash_ies_16_2_0";flash_name="ies_16_2_0_flash_";       EPE=1;GVCP=0;break;;
    0) exit;;
    * ) echo "Please enter a valid selection.";
  esac

done
}
copy_resulting_files() {

if [ $GVCP -ne 0 ] ; then
	NEW_DIR_NAME="$(date '+%d-%b-%Y')_wGVCP"
elif [ $EPE -ne 0 ] ; then
	NEW_DIR_NAME="$(date '+%d-%b-%Y')_wEPE"
else
	NEW_DIR_NAME="$(date '+%d-%b-%Y')"
fi

SRC1="./obj/$switch_device.*"
SRC2="./tools/$flash_name*"

BUILDS_DIR="../../../../Builds"
DEVICE_DIR="$BUILDS_DIR/$switch_device"
NEW_DIR="$DEVICE_DIR/$NEW_DIR_NAME"

echo $NEW_DIR
mkdir -p $DEVICE_DIR
mkdir -p $NEW_DIR
cp $SRC1 $NEW_DIR/
cp $SRC2 $NEW_DIR/

echo "Current User: $USER"
if [[ $USER == "jalden" ]]; then
    echo "Sending files "
    # scp obj/iES-8.dat jalden@scratchy:/srv/tftp/
    scp ./obj/$switch_device.dat jalden@scratchy:/srv/tftp/
fi


mv *_build.txt $NEW_DIR/
mv *_warnings.txt $NEW_DIR/
mv *_errors.txt $NEW_DIR/
}

main_menu $1;
print_out_preparded_list
sed -i "s/VNS_VERSION_MAJOR\s\+[[:digit:]]\+/VNS_VERSION_MAJOR   $VNS_VERSION_MAJOR/g" ../vtss_appl/ies_2_0/vns_types.h
sed -i "s/VNS_VERSION_MINOR\s\+[[:digit:]]\+/VNS_VERSION_MINOR   $VNS_VERSION_MINOR/g" ../vtss_appl/ies_2_0/vns_types.h


sed -i "s/VNS_VERSION_COMMON_IP\s\+[[:digit:]]\+/VNS_VERSION_COMMON_IP   $VNS_VERSION_COMMON_IP/g" ../vtss_appl/ies_2_0/vns_types.h
sed -i "s/VNS_VERSION_PCKG_ID\s\+[[:digit:]]\+/VNS_VERSION_PCKG_ID   $VNS_VERSION_PCKG_ID/g" ../vtss_appl/ies_2_0/vns_types.h


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

current_task="Making clobber/mrproper....."
echo $current_task
make mrproper
test_for_failure $? 

current_task="Creating link....."
echo $current_task
ln -s configs/$reference_device config.mk
test_for_failure $? 

current_task="Making redboot....."
echo $current_task
make redboot -j8 &> ./redboot_build.txt
test_for_failure $? 
cat ./redboot_build.txt | grep warning: > redboot_warnings.txt
cat ./redboot_build.txt | grep error: > redboot_errors.txt

current_task="Making CEServices....."
echo $current_task
make -j8 &> ./ceservices_build.txt
test_for_failure $? 
cat ./ceservices_build.txt | grep warning: > ceservices_warnings.txt
cat ./ceservices_build.txt | grep error: > ceservices_errors.txt

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
make $flash_device &> ./flash_image_build.txt
test_for_failure $? 
cat ./flash_image_build.txt | grep warning: > flash_image_warnings.txt
cat ./flash_image_build.txt | grep error: > flash_image_errors.txt

#END

current_task="returning to ../build....."
echo $current_task
cd ..
test_for_failure $? 

current_task="copying resulting files....."
echo $current_task
copy_resulting_files
test_for_failure $? 


exit 0

