/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef _SGXERRATA_KM_H_
#define _SGXERRATA_KM_H_

#if defined(SGX520) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 100
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 111
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX520 Core Revision unspecified"
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX530) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_22934
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 111
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 1111
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 120
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 121
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 125
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 130
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX530 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
#endif
        #endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX531) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 101
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX531 Core Revision unspecified"
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if (defined(SGX535) || defined(SGX535_V1_1)) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 112
		#define FIX_HW_BRN_23281
		#define FIX_HW_BRN_23410 
		#define FIX_HW_BRN_22693
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_22997
		#define FIX_HW_BRN_23030
	#else
	#if SGX_CORE_REV == 113
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_23281
		#define FIX_HW_BRN_23944
		#define FIX_HW_BRN_23410
	#else
	#if SGX_CORE_REV == 121
		#define FIX_HW_BRN_22934	
		#define FIX_HW_BRN_23944
		#define FIX_HW_BRN_23410
	#else
	#if SGX_CORE_REV == 126
		#define FIX_HW_BRN_22934	
	#else	
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX535 Core Revision unspecified"

	#endif
	#endif
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX540) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 101
		#define FIX_HW_BRN_25499
		#define FIX_HW_BRN_25503
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_25503
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == 120
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == 121
		#define FIX_HW_BRN_28011
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == 130
		#define FIX_HW_BRN_34028
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX540 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX541) && !defined(SGX_CORE_DEFINED)
	#if defined(SGX_FEATURE_MP)
		
		#define SGX_CORE_REV_HEAD	0
		#if defined(USE_SGX_CORE_REV_HEAD)
			
			#define SGX_CORE_REV	SGX_CORE_REV_HEAD
		#endif

		#if SGX_CORE_REV == 100
			#define FIX_HW_BRN_27270
			#define FIX_HW_BRN_28011
			#define FIX_HW_BRN_27510
			
		#else
		#if SGX_CORE_REV == SGX_CORE_REV_HEAD
			
		#else
			#error "sgxerrata.h: SGX541 Core Revision unspecified"
		#endif
		#endif
		
		#define SGX_CORE_DEFINED
	#else 
		#error "sgxerrata.h: SGX541 only supports MP configs (SGX_FEATURE_MP)"
	#endif 
#endif

#if defined(SGX543) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 113
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_29997
		#define FIX_HW_BRN_30954
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_32044 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 122
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_29997
		#define FIX_HW_BRN_30954
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
			
	#else
	#if SGX_CORE_REV == 1221
		#define FIX_HW_BRN_29954
        #define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_31671		
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32044
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
			
	#else
	#if SGX_CORE_REV == 140
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_30954
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#define FIX_HW_BRN_33920
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
			
	#else
	#if SGX_CORE_REV == 1401
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_30954
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
		#define FIX_HW_BRN_31542
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#define FIX_HW_BRN_33920
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
			
	#else
	#if SGX_CORE_REV == 141
		#define FIX_HW_BRN_29954
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31671 
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
			
	#else
	#if SGX_CORE_REV == 142
		#define FIX_HW_BRN_29954
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31671 
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
			
	#else
	#if SGX_CORE_REV == 211
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
			
	#else
	#if SGX_CORE_REV == 2111
		#define FIX_HW_BRN_30982 
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31620
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_31542
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
			
	#else
	#if SGX_CORE_REV == 213
		#define FIX_HW_BRN_31272
		#if defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_31425
		#endif
		#define FIX_HW_BRN_31671 
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
			
	#else
	#if SGX_CORE_REV == 216
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
	#if SGX_CORE_REV == 302
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
	#if SGX_CORE_REV == 303
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
		#error "sgxerrata.h: SGX543 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX544) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 100
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
	#else
	#if SGX_CORE_REV == 102
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_31272
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 103
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_31272
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 104
		#define FIX_HW_BRN_29954
		#define FIX_HW_BRN_31093
		#define FIX_HW_BRN_31195
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_31278
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
		#define FIX_HW_BRN_31542 
 		#define FIX_HW_BRN_31620
		#define FIX_HW_BRN_31671 
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_32044 
		#define FIX_HW_BRN_32085 
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else	
	#if SGX_CORE_REV == 105
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 106
		#define FIX_HW_BRN_31272
 		#define FIX_HW_BRN_31780
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_31272
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 112
		#define FIX_HW_BRN_31272
		#define FIX_HW_BRN_33920
	#else
	#if SGX_CORE_REV == 114
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
	#if SGX_CORE_REV == 115
 		#if defined(SGX_FEATURE_MP)
 			#define FIX_HW_BRN_31425
 		#endif
 		#define FIX_HW_BRN_31780
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
		#error "sgxerrata.h: SGX544 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX545) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 100
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_27266
		#define FIX_HW_BRN_27456
		#define FIX_HW_BRN_29702
		#define FIX_HW_BRN_29823
	#else
	#if SGX_CORE_REV == 109
		#define FIX_HW_BRN_29702
		#define FIX_HW_BRN_29823
		#define FIX_HW_BRN_31939
	#else
	#if SGX_CORE_REV == 1012
		#define FIX_HW_BRN_31939
	#else
	#if SGX_CORE_REV == 1013
		#define FIX_HW_BRN_31939
		#define FIX_HW_BRN_SAMPLE_CACHE
	#else
	#if SGX_CORE_REV == 10131
		#define FIX_HW_BRN_SAMPLE_CACHE
	#else
	#if SGX_CORE_REV == 1014
		#define FIX_HW_BRN_SAMPLE_CACHE
	#else
	#if SGX_CORE_REV == 10141
		#define FIX_HW_BRN_SAMPLE_CACHE
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		
	#else
		#error "sgxerrata.h: SGX545 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX554) && !defined(SGX_CORE_DEFINED)
	
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 1251
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
		
	#else	
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING) && defined(SGX_FEATURE_MP)
			#define FIX_HW_BRN_33657
		#endif
	#else
		#error "sgxerrata.h: SGX554 Core Revision unspecified"
	#endif
	#endif
	
	#define SGX_CORE_DEFINED
#endif

#if !defined(SGX_CORE_DEFINED)
#if defined (__GNUC__)
	#warning "sgxerrata.h: SGX Core Version unspecified"
#else
	#pragma message("sgxerrata.h: SGX Core Version unspecified")
#endif
#endif

#endif 

