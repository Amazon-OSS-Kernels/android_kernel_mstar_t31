#!/bin/bash

if [ $# -eq 1 ] && [ -d $1 ]
then
    AN_ROOT_PATH=`realpath $1`
else
    AN_ROOT_PATH=`pwd`
fi


function copy_safe() {
    src=${1}
    dst=${2}
    [ -e ${src} ] && mv ${src} ${dst}
}

function link_safe() {
    src=${1}
    dst=${2}
    if [ -L ${dst} ]; then
        rm -rf ${dst}
    fi
    ln -s ${src} ${dst}
}

KERN_LOC=${AN_ROOT_PATH}/kernel/mstar/t31/4.9
MISDK_CUSTOM_BUILD_ENTRY=${KERN_LOC}/../../../../vendor/mediatek/proprietary_tv/apollo/linux_core/misdk_custom/build
make -C ${MISDK_CUSTOM_BUILD_ENTRY}/patch/mak kernel
MST_DRVROOT=${KERN_LOC}/drivers
echo -e "genlink info:sourece build 4.9(MST_DRVROOT,KERN_LOC,AN_ROOT_PATH)=($MST_DRVROOT,$KERN_LOC,$AN_ROOT_PATH)\n"
link_safe ${MST_DRVROOT}/mstar2/drv/cpu/arm ${KERN_LOC}/arch/arm/arm-boards
link_safe ${MST_DRVROOT}/mstar2/drv/cpu/arm64 ${KERN_LOC}/arch/arm64/arm-boards
# link_safe ../generated/autoconf.h ./include/linux/autoconf.h
link_safe ${MST_DRVROOT}/mstar2/Kconfig ${KERN_LOC}/arch/mips/Kconfig_kdrv
# Copy header files
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ECN_U.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ECN.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ecn_L.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ecn.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_TTL_U.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_TTL.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ttl_L.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv4/ipt_ttl.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv6/ip6t_HL_U.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv6/ip6t_HL.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter_ipv6/ip6t_hl_L.h  ${KERN_LOC}/include/uapi/linux/netfilter_ipv6/ip6t_hl.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_CONNMARK_U.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_CONNMARK.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_connmark_L.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_connmark.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_DSCP_U.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_DSCP.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_dscp_L.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_dscp.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_MARK_U.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_MARK.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_mark_L.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_mark.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_RATEEST_U.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_RATEEST.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_rateest_L.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_rateest.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_TCPMSS_U.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_TCPMSS.h
copy_safe ${KERN_LOC}/include/uapi/linux/netfilter/xt_tcpmss_L.h ${KERN_LOC}/include/uapi/linux/netfilter/xt_tcpmss.h
copy_safe ${KERN_LOC}/net/netfilter/xt_DSCP_U.c ${KERN_LOC}/net/netfilter/xt_DSCP.c
copy_safe ${KERN_LOC}/net/netfilter/xt_dscp_L.c ${KERN_LOC}/net/netfilter/xt_dscp.c
copy_safe ${KERN_LOC}/net/netfilter/xt_HL_U.c ${KERN_LOC}/net/netfilter/xt_HL.c
copy_safe ${KERN_LOC}/net/netfilter/xt_hl_L.c ${KERN_LOC}/net/netfilter/xt_hl.c
copy_safe ${KERN_LOC}/net/netfilter/xt_RATEEST_U.c ${KERN_LOC}/net/netfilter/xt_RATEEST.c
copy_safe ${KERN_LOC}/net/netfilter/xt_rateest_L.c ${KERN_LOC}/net/netfilter/xt_rateest.c
copy_safe ${KERN_LOC}/net/netfilter/xt_TCPMSS_U.c ${KERN_LOC}/net/netfilter/xt_TCPMSS.c
copy_safe ${KERN_LOC}/net/netfilter/xt_tcpmss_L.c ${KERN_LOC}/net/netfilter/xt_tcpmss.c

true
