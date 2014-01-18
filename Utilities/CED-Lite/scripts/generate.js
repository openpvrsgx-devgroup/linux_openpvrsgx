/******************************************************************************
 * Copyright 2012 Intel Corporation All Rights Reserved.
 *
 * The source code, information and material ("Material") contained herein is
 * owned by Intel Corporation or its suppliers or licensors, and title to such
 * Material remains with Intel Corporation or its suppliers or licensors. The 
 * Material contains proprietary information of Intel or its suppliers and 
 * licensors. The Material is protected by worldwide copyright laws and treaty 
 * provisions. No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed or disclosed in any way 
 * without Intel's prior express written permission. No license under any patent,
 * copyright or other intellectual property rights in the Material is granted to 
 * or conferred upon you, either expressly, by implication, inducement, estoppel 
 * or otherwise. Any license under such intellectual property rights must be 
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's suppliers 
 * or licensors in any way.
 *
 ********************************************************************************/

var mainTemplate = new Object();
var configTemplate = new Object();
var port = new Object();
var sharedTemplate = new Object();

var special='$';

var customDTDs = new Object();


function generate()
{
	resetTemplateArrays();
	
	mainTemplate["default_configuration"] = '1';	
	
	configTemplate['config_name'] = 'no_name';
	
	sharedTemplate['CHIPSET_ALL'] = "ALL";
	sharedTemplate['CONFIG_ID'] = "1";
	
	port['lvds'] = new Object();	
	
	port['sdvo'] = new Object();	

	
	configInput(); /* configureInput.js */
	
	return processMainTemplate(); /* processTemplates.js */
}
		