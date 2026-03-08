/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2019 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <command.h>
#include <common.h>
#include <MsDebug.h>
#include <drvIIC.h>
#include <drvGPIO.h>
#include <bootlogo/iniparser.h>
#include <MsUtility.h>
#include <MsVfs.h>
#include <CusConfig.h>
#include <idme.h>

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define BUFFER_SIZE        128
#define REGBUG_LEN         32
#define REG_WRITE_DELAY_MS 100
#define NVM_DELAY          50 //5 seconds

#define TCON_CONFIG_SECTION "TCON_POWER_IC_CONFIG"
#define TCON_INI_FILE "TCON_CONFIG_FILE"
#define TCON_INI_DEFVAL ""

//#define DEFAULT_VCOM_VAL 0x7E
//#define TCON_VCOM_ADDR   0x8
//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Extern Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Private Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void Get_VcomValues(char *valueStr, int *Vcom1, int *Vcom2)
{
   char tmp[32];
   unsigned char  idx = 0, ct = 0;

   if (valueStr  == NULL)
       return;

   while ((valueStr[idx] != ',') && (valueStr[idx] != '\0') && (idx < 32)) {
	   tmp[idx] = valueStr[idx];
	   idx++;
   }

   //value string read from IDME is hex value without 0x prefix
   if (valueStr[idx] == '\0' ) {
	 tmp[idx] = '\0';
            *Vcom1 = (int)strtol(tmp, NULL, 16);
	 *Vcom2 = 0;
	 return;
   }

   if (valueStr[idx] == ',') {
       tmp[idx] = '\0';
       *Vcom1 = (int)strtol(tmp, NULL, 16);
       idx++; //skip ,
       ct = idx;
       while (valueStr[idx] != '\0' && idx < 32) {
          tmp[ idx-ct ] = valueStr[idx];
          idx++;
       }

       if (valueStr[idx] == '\0') {
	   tmp[idx-ct] = '\0';
           *Vcom2 = (int)strtol(tmp, NULL, 16);
       }
   }
}

#define SY76XX_NVM_WRITE_BIT  0x8
#define SY76XX_RESET          0x12
#define SY76XX_RESET_STATUS   0x10
#define MAX_LOOP_CT           10

static int wait_to_store(unsigned short i2c_addr, const char *section )
{
   int loop = 0, ret = 0;
   unsigned char regVal[1];
   unsigned char regCtrl[1] = {0};

   do {
         if (!strncmp(section, "Gamma", 5))
             mdelay(5*REG_WRITE_DELAY_MS);
	 else
             mdelay(2*REG_WRITE_DELAY_MS);

         ret = MDrv_IIC_ReadBytesPort0(i2c_addr + 1, 1, regCtrl, 1, regVal);
         if (!ret) {
             UBOOT_INFO("[TCON: %s] iic read failed, ret = %d, retry\n", section, ret);
	     loop++;
	     continue;
         }
         else {
             UBOOT_INFO("[TCON: %s] wait for NVM write to complete\n", section);
	     loop++;
         }
   } while((regVal[0] & SY76XX_NVM_WRITE_BIT) && (loop < NVM_DELAY));// check bit 4 for completion and max wait time is 5 seconds

   if (loop >= NVM_DELAY) {
       UBOOT_ERROR("[TCON: %s] NVM write time out\n", section);
       return 1;
   }
   else {
       UBOOT_INFO("[TCON: %s] NVM write completed\n", section);
       //even the status show the write is complete, it actually still not ready to reload
       //each register takes 17 ms wite to NVM so it will take about 400 ms to complete.
       mdelay(REG_WRITE_DELAY_MS);
       //PMU need to do power reset in order to output 12V to gamma. This looks like a
       //chip hardware bug
       if (!strncmp(section, "PMU", 3)){
	   Panel_VCC_OFF();
           //mdrv_gpio_set_low(PAD_SAR2);
           UBOOT_INFO("[TCON: %s] NVM power off reset to ouput 12V.\n", section);
           mdelay(4*REG_WRITE_DELAY_MS);
	   Panel_VCC_ON();
           //mdrv_gpio_set_high(PAD_SAR2);
       }
       return 0;
   }
}

static int reload_register_values(unsigned short i2c_addr, const char *section)
{
    unsigned char regVal[1] = {SY76XX_RESET}, value[1];
    unsigned char regCtrl[1] = {0};
    int ret = 0, loop = 0;

    UBOOT_INFO("[TCON: %s] reset to load register value from NVM\n", section);

    do {
         mdelay(REG_WRITE_DELAY_MS);
         ret = MDrv_IIC_WriteBytesPort0(i2c_addr, 1, regCtrl, 1, regVal);
         if (!ret) {
             UBOOT_INFO("[TCON: %s] iic write  error ret = %d, retry.\n", section, ret);
	     loop++;
	     continue;
	 }
    } while ((ret == 0) && (loop < MAX_LOOP_CT));

    if (loop >= MAX_LOOP_CT ) {
       UBOOT_ERROR("[TCON: %s] failed to reload registers, iic write  error , retry ret %d\n", section, ret);
       return -1;
    }

    loop = 0;
    if (!strncmp(section, "Gamma", 5)){
        mdelay(4*REG_WRITE_DELAY_MS);
        UBOOT_INFO("[TCON: %s] wait reset to complete.", section);
    }

    do {
          mdelay(REG_WRITE_DELAY_MS);
	  ret = MDrv_IIC_ReadBytesPort0(i2c_addr + 1, 1, regCtrl, 1, value);
          if (!ret) {
                 UBOOT_INFO("[TCON: %s] iic read  error ret %d, rety\n", section, ret);
		 loop++;
                 continue;
          }
          else {
               UBOOT_INFO("[TCON: %s] wait to load register values from NVM\n", section);
	       loop++;
          }
    } while((value[0] & SY76XX_RESET_STATUS) && (loop < MAX_LOOP_CT));

    if (loop >= MAX_LOOP_CT ) {
        UBOOT_ERROR("[TCON: %s] failed to reload register values\n", section);
	return 1;
    }
    else {
        UBOOT_INFO("[TCON: %s] reload register values completed\n", section);
	return 0;
    }
}

static int read_and_compare_tcon_reg(const char *Section, int *skipVCOM, unsigned char *currentVal)
{
    char Buffer[BUFFER_SIZE * 8];
    int idx, i2c_mode;
    int ret = 0;
    int length = 0;
    int uVcomReg = -1;
    int count = 0;
    int updateFlag = 0;
    const int u16AddrCnt = 1;
    unsigned short vcom_def_val = 0;
    unsigned short u16I2C_Addr;
    unsigned char pu8rBuf[REGBUG_LEN];
    unsigned char u8AddrBuf[REGBUG_LEN];
    unsigned char u8ValueBuf[REGBUG_LEN];

    length = Profile_GetInteger(Section, "Reg_Length", 0);
    UBOOT_DEBUG("[TCON: %s] Reg_Length %d\n", Section, length);
    //the sectoin may not exist
    if (length == 0 )
	return updateFlag;

    Profile_GetString(Section, "Reg_Addr_Table", "", Buffer, sizearray(Buffer));
    if (!Profile_TranslateU8Array(Buffer, length, u8AddrBuf))
        return -1;

    Profile_GetString(Section, "Reg_Value_Table", "", Buffer, sizearray(Buffer));
    if (!Profile_TranslateU8Array(Buffer, length, u8ValueBuf))
        return -1;

    u16I2C_Addr = (unsigned short)Profile_GetInteger(Section, "I2C_Addr", 0);
    UBOOT_DEBUG("[TCON: ] I2C_Addr 0x%x\n", u16I2C_Addr);

    uVcomReg = (int)Profile_GetInteger(Section, "Vcom_Addr", -1);
    UBOOT_DEBUG("[TCON: %s] Vcom_Addr %d\n", Section, uVcomReg);

    // Read, if the tcon register is not contiguous, use single byte read.
    i2c_mode = (int)Profile_GetInteger(Section, "I2C_WR_Model", 0);
    if (i2c_mode == 1){
        for ( idx=0; idx<length; idx++ ) {
           ret = MDrv_IIC_ReadBytesPort0(u16I2C_Addr + 1, u16AddrCnt, &u8AddrBuf[idx], 1, &pu8rBuf[idx]);
	   if (!ret) {
              UBOOT_ERROR("[TCON: %s] iic read error ret %d at 0x%02x\n", Section, ret, idx);
	      return -1;
	   }
	}
    }
    else {
        UBOOT_DEBUG("[TCON: ] read addr 0x%02x, length %d\n", u8AddrBuf[0], length);
        ret = MDrv_IIC_ReadBytesPort0(u16I2C_Addr + 1, u16AddrCnt, u8AddrBuf, length, pu8rBuf);
        if (!ret) {
            UBOOT_ERROR("[TCON: ] I2C read error ret %d\n", ret);
            return -1;
        }
    }
    // Check value of VCOM register. Skip VCOM REG if value is not default value.
    // TODO: this applies to TCON with only one VCOM register, some TCON has more than one VCOM register (VCOM value > 8bit)
    // or there are two VCOM values each has more then 8 bit. one way to fix this is to specify a list of vcom values in ini file
    // just as what address table and value table do. here only look at the first one.
    if (-1 != uVcomReg ) {
        vcom_def_val = (unsigned short)Profile_GetInteger(Section, "Vcom_Default_Value", 0);
        UBOOT_INFO("[TCON: %s] Vcom_Default_Value 0x%x\n", Section, vcom_def_val);
        for(count = 0; count < length; count++) {
            if ( (uVcomReg == u8AddrBuf[count]) && (pu8rBuf[count] != vcom_def_val) ) {
                UBOOT_INFO("[TCON: %s] VCOM 0x%02x:  0x%02x, default value: 0x%02x", Section, uVcomReg,
                            pu8rBuf[count], vcom_def_val);
                *skipVCOM = 1;
                UBOOT_INFO("[TCON: %s] skip vcom flag %d\n", Section, *skipVCOM);
                *currentVal = pu8rBuf[count]; //use calibrated value
                u8ValueBuf[count] = pu8rBuf[count];
                break;
            }
        }
    }

    ret = memcmp(pu8rBuf, u8ValueBuf, length);
    if (ret) {
        updateFlag = 1;
        UBOOT_INFO("Reg mismatch set updateFlag: %d Section: %s\n", updateFlag, Section);
    }

    for (idx=0; idx < length; idx++)
        UBOOT_DEBUG("[TCON: %s] check register values 0x%02x, value 0x%02x\n", Section, u8AddrBuf[idx], pu8rBuf[idx]);
    return updateFlag;
}

static int write_tcon_reg(const char *Section, int store, int skipVCOM, unsigned char currentVal, int *reboot)
{
    char Buffer[BUFFER_SIZE * 8];
    int val_len;
    int length = 0;
    int ret = 0;
    int vcom1_value = 0, vcom2_value = 0;
    int count = 0;
    int uVcomReg = -1, uVcomRegLen = 0, vcom_idx = -1, i2c_mode = 0;
    int idx = 0, wp_gpio = -1, wp_level = -1;
    const int u16AddrCnt = 1;
    unsigned char pu8Addr[1];
    unsigned char pu8wBuf[1];
    unsigned char u8AddrBuf[REGBUG_LEN]   = {0};
    unsigned char u8ValueBuf[REGBUG_LEN]  = {0};
    unsigned char u8ValReload[REGBUG_LEN] = {0};
    unsigned short u16I2C_Addr;

    length = Profile_GetInteger(Section, "Reg_Length", 0);
    UBOOT_INFO("[TCON: %s] Reg_Length %d\n", Section, length);

    Profile_GetString(Section, "Reg_Addr_Table", "", Buffer, sizearray(Buffer));
    if (!Profile_TranslateU8Array(Buffer, length, u8AddrBuf))
        return -1;

    Profile_GetString(Section, "Reg_Value_Table", "", Buffer, sizearray(Buffer));
    if (!Profile_TranslateU8Array(Buffer, length, u8ValueBuf))
        return -1;

    uVcomReg = (int)Profile_GetInteger(Section, "Vcom_Addr", -1);
    UBOOT_DEBUG("[TCON: %s] Vcom_Addr %d\n", Section, uVcomReg);
    //validate vcom register address
    for (count = 0 ; count < length ; count++) {
        if ( uVcomReg == u8AddrBuf[count] ) {
            UBOOT_INFO("[TCON: %s] reg 0x%02x (VCOM) is 0x%02x.\n", Section, uVcomReg, currentVal);
	    vcom_idx = count;
            break;
        }
    }

    u16I2C_Addr = (unsigned short)Profile_GetInteger(Section, "I2C_Addr", 0);
    UBOOT_DEBUG("[TCON: ] I2C_Addr 0x%x\n", u16I2C_Addr);
    // Check VCOM is default.
    if ( 1 == skipVCOM ) {
       if ( vcom_idx > -1 ){
            u8ValueBuf[vcom_idx] = currentVal;
       }
    }
    else {
       long eeprom = Profile_GetInteger(Section, "Write_Reg_Boot_On", -1l);
       UBOOT_INFO("[TCON: %s] power on register write check %ld\n", Section, eeprom);

       // for all non-EEPROM TCONs, currentVal will be 0 and skipVCOM will be 0 then check IDME
       if ( (vcom_idx > -1) && (eeprom == 1l) ) {
           uVcomRegLen = (int)Profile_GetInteger(Section, "Vcom_Value_Len", 0);
           UBOOT_INFO("[TCON: %s] Vcom Register value length %d\n", Section, uVcomRegLen);
	   val_len = Profile_GetString(Section, "Vcom_Idme_Name", "", Buffer, sizearray(Buffer));
	   if ( val_len && (uVcomRegLen != 0) ) {
	       char value[32];
	       if ( 0 == idme_get_var_external(Buffer, value, 32) ) {
                   UBOOT_INFO("[TCON: %s] Vcom string from IDME: %s\n", Section, value);
		   Get_VcomValues(value, &vcom1_value, &vcom2_value);
	           if ( (vcom1_value > 0) || (vcom2_value > 0)) {
	              if (uVcomRegLen == 8) { //not any case for now
                          u8ValueBuf[vcom_idx] = vcom1_value;
                          u8ValueBuf[vcom_idx +1] = vcom2_value;
	              }
                      else if ((uVcomRegLen > 8) && (uVcomRegLen <= 12)){                                    //RT6966, maximal VOCM lenght is 12 bits
	                  u8ValueBuf[vcom_idx]   = (unsigned char)(vcom1_value >> 4);                        //bit [11:4] for VCOM1
                          u8ValueBuf[vcom_idx +1 ] = (unsigned char)((vcom1_value << 4) | (vcom2_value >> 8)); //bit [3:0] for VCOM1 and [11:8] for VCOM2
                          u8ValueBuf[vcom_idx + 2] = (unsigned char)(vcom2_value & 0xFF) ;                     //bit [7:0] for VCOM2
			  UBOOT_INFO("[TCON: %s] Vcom Register values %d, %d, %d\n", Section, u8ValueBuf[vcom_idx], u8ValueBuf[vcom_idx+1],u8ValueBuf[vcom_idx+2]);
	              }
	           }
                   else
                      UBOOT_ERROR("[TCON: %s] VCOM values are zeros!\n", Section);
	       }
	       else
                  UBOOT_ERROR("[TCON: %s] VCOM1 value from IDME not found !\n", Section);
	   }
	   else
               UBOOT_ERROR("[TCON: %s] VCOM value length is wrong or the name is not found in IDME!\n", Section);
       }
    }
    // gpio is already initialized and the default status is output
    if ( (wp_gpio = Profile_GetInteger(Section, "WP_Pin", -1)) >= 0 ) {
        wp_level = Profile_GetInteger(Section, "WP_Level", -1);
	wp_gpio++; //seems that MTk has two schema and the interanl one starts from 1
        if ( wp_level == 1)
	   mdrv_gpio_set_low(wp_gpio);
        else
	   mdrv_gpio_set_high(wp_gpio);
        UBOOT_INFO("[TCON: %s] disable TCON register write protection WP pin: %d level:%d\n", Section, wp_gpio, mdrv_gpio_get_level(wp_gpio));
    }
    // there are two different modes for I2c write, multiple or single
    // regiter address has to be contiguous in order to use multiple bytes write, otherwise use
    // single byte.
    UBOOT_INFO("[TCON: %s] write addr 0x%02x, length %d\n", Section, u8AddrBuf[0], length);
    i2c_mode = (int)Profile_GetInteger(Section, "I2C_WR_Model", 0);
    if (i2c_mode == 1){
        for ( idx=0; idx<length; idx++) {
           UBOOT_DEBUG("[TCON: %s] iic write register 0x%02x, value 0x%02x\n", Section, u8AddrBuf[idx], u8ValueBuf[idx]);
           ret = MDrv_IIC_WriteBytesPort0(u16I2C_Addr, u16AddrCnt, &u8AddrBuf[idx], 1, &u8ValueBuf[idx]);
	   if (!ret) {
              UBOOT_ERROR("[TCON: %s] iic write error ret %d\n", Section, ret);
	      return -1;
	   }
	}
    }
    else
       ret = MDrv_IIC_WriteBytesPort0(u16I2C_Addr, u16AddrCnt, u8AddrBuf, length, u8ValueBuf); //multiple bytes write

    // Workaroud for SY7687
    val_len = Profile_GetString(Section, "Name", "", Buffer, sizearray(Buffer));
    if(!strncmp(Buffer, "SY7687", sizeof(Buffer))) {
        UBOOT_DEBUG("[TCON: %s] Name %s\n", Section, Buffer);
        ret = MDrv_IIC_WriteBytesPort0(u16I2C_Addr, u16AddrCnt, u8AddrBuf+1, length-1, u8ValueBuf+1);
    }
    if (!ret) {
        UBOOT_ERROR("[TCON: %s] iic write error ret %d\n", Section, ret);
        return -1;
    }

    // some TCON does not have interanl EEPROM to store DAC register settings and
    // the Write_Cmd_Addr will be set to -1 in this case
    if (store) {
        pu8Addr[0] = Profile_GetInteger(Section, "Write_Cmd_Addr",  0);
        pu8wBuf[0] = Profile_GetInteger(Section, "Write_Cmd_Value", 0);
	if ( pu8wBuf[0] != 0x0  ) {
            UBOOT_INFO("[TCON: %s] commit register write  0x%02x, data 0x%02x\n", Section, pu8Addr[0], pu8wBuf[0]);
            ret = MDrv_IIC_WriteBytesPort0(u16I2C_Addr, u16AddrCnt, pu8Addr, 1, pu8wBuf);
            if (!ret) {
               UBOOT_ERROR("[TCON: %s] iic write error ret %d\n", Section, ret);
               return -1;
            }
	}
        //wait for NVM write to completed.
        if( (!strncmp(Buffer, "SY7634PRC", sizeof(Buffer))) || (!strncmp(Buffer, "SY7687QEC", sizeof(Buffer))) ) {
            if (0 !=  wait_to_store(u16I2C_Addr, Section )) {
                 UBOOT_ERROR("[TCON: %s] failed to write register values to NVM %d\n", Section, ret);
                 return -1;
	    }
	    else {
                   ret = reload_register_values(u16I2C_Addr, Section);
		   if (ret != 0 ) {
                       UBOOT_ERROR("[TCON: %s] failed to reload register values %d\n", Section, ret);
		       return -1;
		   }
		   else {
                       if (i2c_mode == 1) {
                           for ( idx=0; idx<length; idx++) {
                               ret = MDrv_IIC_ReadBytesPort0(u16I2C_Addr + 1, u16AddrCnt, &u8AddrBuf[idx], 1, &u8ValReload[idx]);
	                       if (!ret) {
                                   UBOOT_ERROR("[TCON: %s] iic read error ret %d\n", Section, ret);
	                           return -1;
	                       }
	                    }
                       }
                       else {
                           ret = MDrv_IIC_ReadBytesPort0(u16I2C_Addr + 1, u16AddrCnt, u8AddrBuf, length, u8ValReload);
                           if (!ret) {
                               UBOOT_ERROR("[TCON: %s] fail to read back regs values\n", Section);
	                       return -1;
                           }
                       }
		   }
                   for (idx=0; idx < length; idx++)
                       UBOOT_DEBUG("[TCON: %s] read back reg: 0x%02x, value: 0x%02x, expect: 0x%02x\n", Section, u8AddrBuf[idx], u8ValReload[idx], u8ValueBuf[idx]);

                   ret = memcmp(u8ValReload, u8ValueBuf, length);
                   if (ret) {
                       UBOOT_ERROR("[TCON: %s] register values do not match, ret %d\n", Section, ret);
		       return -1;
                   }
		   else
                       UBOOT_INFO("[TCON: %s] register values update completed", Section);
	           *reboot = 0; //no need to reboot for new 50 inch model, instead using TCON reset.
	    }
	}
	else 
	   *reboot = 1; //need reboot for old models
    }

    if ( wp_gpio > 0 ) {
        if ( wp_level == 1)
	   mdrv_gpio_set_high(wp_gpio);
        else
	   mdrv_gpio_set_low(wp_gpio);
        UBOOT_DEBUG("[TCON: %s] enable TCON register write protection, WP pin:%d, level:%d\n", Section, wp_gpio, mdrv_gpio_get_level(wp_gpio));
    }
    return 0;
}

int do_tcon_upgrade(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    UBOOT_TRACE("TCON: ] IN\n");

    char *script = NULL;
    char *Customer_ini_file = NULL;
    char InPath[BUFFER_SIZE]="\0";
    unsigned int filesize;
    char *s_tmp;
    long ac_on_force = 0l; //program TCON register at AC on
    int ret;
    int updateFlag = 0;
    int skipVCOM = 0;
    int ePort = 0, reboot = 0;
    unsigned char currentVal = 0;

    // Load Customer_1 ini file.
    Customer_ini_file = getenv ("customer_ini_path");
    // remove partition name from path
    s_tmp = Customer_ini_file;
    while (*s_tmp == '/') s_tmp++;
    s_tmp = strstr((const char*)s_tmp,"/");
    UBOOT_DEBUG("[TCON:] customer_ini file = %s\n", s_tmp);

    if (s_tmp != NULL) {
        script = (char *)loadscript(s_tmp, &filesize);
        if (script == NULL) {
            // mount tvconfig
            UBOOT_DEBUG("[TCON: ] mount %s\n", CONFIG);
            if(vfs_mount(CONFIG)!= 0) {
                UBOOT_ERROR("[TCON: ] mount %s fail\n", CONFIG);
                return 1;
            }
            script = (char *)loadscript(s_tmp, &filesize);
        }
    }

    if (script == NULL) {
        UBOOT_ERROR("[TCON: ] Unable to load %s\n", s_tmp);
        return 1;
    }

    // Get TCON ini file path
    Profile_Init((char *)script, filesize);
    if (Profile_GetString(TCON_CONFIG_SECTION, TCON_INI_FILE, TCON_INI_DEFVAL, InPath, sizearray(InPath))) {
        // Load TCON ini file
        UBOOT_DEBUG("[TCON: ] INI FILE PATH:%s\n", InPath);
        script = (char *)loadscript(InPath, &filesize);
    }
    else {
        UBOOT_ERROR("[TCON: ] Unable to find [%s] %s\n", TCON_CONFIG_SECTION, TCON_INI_FILE);
        return 1;
    }

    if(script != NULL) {
        Profile_Init((char *)script,filesize);

        ePort = Profile_GetInteger("IIC", "IIC_Port", E_HWI2C_PORT2_1);

        UBOOT_DEBUG("[TCON: IIC] Init IIC port %d\n", ePort);
        MDrv_IIC_InitPort0(ePort);

        ac_on_force = Profile_GetInteger("PMU", "Write_Reg_Boot_On", 0l);
        UBOOT_INFO("[TCON: PMU] program TCON register at AC on %ld\n", ac_on_force);
	//set ac_on_force to 1 for those TCONs without interanl EEPROM
	//for which register vaules have to be loaded from ini file
	//everytime after a power cycle.
        if (ac_on_force != 1l) {
            ret = read_and_compare_tcon_reg("PMU", &skipVCOM, &currentVal);
            if (ret == 1) {
                if (write_tcon_reg("PMU", 1, skipVCOM, currentVal, &reboot)) {
                    UBOOT_ERROR("[TCON: PMU] failed to update regs.\n");
	        }
                else
		{
		    updateFlag |= reboot;
                    UBOOT_INFO("[TCON: PMU] regs are updated.\n");
		}
            }
            else if (ret < 0) {
                UBOOT_ERROR("[TCON: PMU] Failed to read tcon Gamma reg.\n");
            }

            ret = read_and_compare_tcon_reg("Gamma", &skipVCOM, &currentVal);
            if (ret == 1) {
                if (write_tcon_reg("Gamma", 1, skipVCOM, currentVal, &reboot)){
                    UBOOT_ERROR("[TCON: Gamma] failed to update regs.\n");
	        }
                else { 
		    updateFlag |= reboot;
                    UBOOT_INFO("[TCON: Gamma] regs are updated.\n");
                }
	    }
            else if (ret < 0) {
                UBOOT_ERROR("[TCON: Gamma] Failed to read tcon PMU reg.\n");
            }

            ret = read_and_compare_tcon_reg("Level_Shift", &skipVCOM, &currentVal);
            if (ret == 1) {
                if (write_tcon_reg("Level_Shift", 1, skipVCOM, currentVal, &reboot)){
                    UBOOT_ERROR("[TCON: Level_Shfit] failed to update regs.\n");
	        }
	        else {
                    updateFlag |= reboot;
                    UBOOT_INFO("[TCON: Level_Shift] regs are updated.\n");
		}
            }
            else if (ret < 0) {
                UBOOT_ERROR("[TCON: Level_Shift] Failed to update tcon Level_Shift reg.\n");
            }
	}
	else {
  	   //force to update all regsiters for TCON without internal EEPROM
	   skipVCOM = 0;
           if (write_tcon_reg("PMU", 0, skipVCOM, currentVal, &reboot)) {
               UBOOT_ERROR("[TCON: PMU] failed to update regs.\n");
           }
           else
               UBOOT_INFO("[TCON: PMU] regs are updated.\n");

           if (write_tcon_reg("Gamma", 0, skipVCOM, currentVal, &reboot)){
               UBOOT_ERROR("[TCON: Gamma] failed to update regs.\n");
           }
           else
               UBOOT_INFO("[TCON: Gamma] regs are updated.\n");

           if (write_tcon_reg("Level_Shift", 0, skipVCOM, currentVal, &reboot)){
               UBOOT_ERROR("[TCON: Level_Shfit] failed to update regs.\n");
           }
           else
               UBOOT_INFO("[TCON: Level_Shift] regs are updated.\n");
	}
	// can't reset the system for RT6966 for it will lose all the register settings
	if ( (ac_on_force != 1l) && (updateFlag) )
              run_command("reset",0);
    }
    else {
        UBOOT_ERROR("[TCON: ] failed to load tcon ini file\n");
        return 1;
    }

    UBOOT_TRACE("[TCON: ] OK\n");
    return 0;
}
