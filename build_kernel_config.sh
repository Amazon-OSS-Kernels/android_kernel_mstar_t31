################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

export BUILD_KERNLE_LOCAL_MK=true
KERNEL_SUBPATH="kernel/mstar/t31/4.9"
DEFCONFIG_NAME="t31_user_defconfig"
TARGET_ARCH="arm64"
TOOLCHAIN_REPO="https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9"
TOOLCHAIN_NAME="aarch64-linux-android-4.9"
TOOLCHAIN_PREFIX="aarch64-linux-android-"
MAKE_DTBS=

# Expected image files are seperated with ":"
KERNEL_IMAGES="arch/arm64/boot/Image:arch/arm64/boot/Image.gz"

################################################################################
# NOTE: You must fill in the following with the path to a copy of an
#       linaro-4.9.3-2014.11-arm-linux-gnueabihf compier
################################################################################
CROSS_COMPILER_PATH=""
