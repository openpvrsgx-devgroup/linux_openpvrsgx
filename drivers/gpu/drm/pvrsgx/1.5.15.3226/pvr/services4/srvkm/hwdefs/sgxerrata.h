/*************************************************************************/ /*!
@Title          SGX HW errata definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Specifies associations between SGX core revisions
                and SW workarounds required to fix HW errata that exist
                in specific core revisions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#ifndef _SGXERRATA_KM_H_
#define _SGXERRATA_KM_H_

/* ignore warnings about unrecognised preprocessing directives in conditional inclusion directives */
/* PRQA S 3115 ++ */

#if defined(SGX520) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX520 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 100
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		/* RTL head - no BRNs to apply */
	#else
		#error "sgxerrata.h: SGX520 Core Revision unspecified"
	#endif
	#endif
	/* signal that the Core Version has a valid definition */
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX530) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX530 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 103
		#define FIX_HW_BRN_22934
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_22934
		#define FIX_HW_BRN_28889
	#else
	#if SGX_CORE_REV == 111
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
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		/* RTL head - no BRNs to apply */
	#else
		#error "sgxerrata.h: SGX530 Core Revision unspecified"
	#endif
	#endif
	#endif
	#endif
	#endif
#endif
        #endif
	/* signal that the Core Version has a valid definition */
	#define SGX_CORE_DEFINED
#endif

#if defined(SGX531) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX531 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 101
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD

	#else
		#error "sgxerrata.h: SGX531 Core Revision unspecified"
	#endif
	#endif

	#define SGX_CORE_DEFINED
#endif

#if (defined(SGX535) || defined(SGX535_V1_1)) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX535 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 111
		#define FIX_HW_BRN_23281
		#define FIX_HW_BRN_23410
		#define FIX_HW_BRN_22693
		#define FIX_HW_BRN_22934
		#define FIX_HW_BRN_22997
		#define FIX_HW_BRN_23030
	#else
	#if SGX_CORE_REV == 1111
		#define FIX_HW_BRN_23281
		#define FIX_HW_BRN_23410
		#define FIX_HW_BRN_22693
		#define FIX_HW_BRN_22934
		#define FIX_HW_BRN_22997
		#define FIX_HW_BRN_23030
	#else
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
		/* RTL head - no BRNs to apply */
	#else
		#error "sgxerrata.h: SGX535 Core Revision unspecified"

	#endif
	#endif
	#endif
	#endif
	#endif
	#endif
	#endif

	#define SGX_CORE_DEFINED
#endif

#if defined(SGX540) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX540 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 101
		#define FIX_HW_BRN_25499
		#define FIX_HW_BRN_25503
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
	#else
	#if SGX_CORE_REV == 110
		#define FIX_HW_BRN_25503
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
	#else
	#if SGX_CORE_REV == 120
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_28011
	#else
	#if SGX_CORE_REV == 121
		#define FIX_HW_BRN_28011
	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		/* RTL head - no BRNs to apply */
	#else
		#error "sgxerrata.h: SGX540 Core Revision unspecified"
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
		#if SGX_CORE_REV == 101

		#else
		#if SGX_CORE_REV == SGX_CORE_REV_HEAD

		#else
			#error "sgxerrata.h: SGX541 Core Revision unspecified"
		#endif
		#endif
		#endif

		#define SGX_CORE_DEFINED
	#else
		#error "sgxerrata.h: SGX541 only supports MP configs (SGX_FEATURE_MP)"
	#endif
#endif

#if defined(SGX543) && !defined(SGX_CORE_DEFINED)
	#if defined(SGX_FEATURE_MP)

		#define SGX_CORE_REV_HEAD	0
		#if defined(USE_SGX_CORE_REV_HEAD)

			#define SGX_CORE_REV	SGX_CORE_REV_HEAD
		#endif

		#if SGX_CORE_REV == 100

		#else
		#if SGX_CORE_REV == SGX_CORE_REV_HEAD

		#else
			#error "sgxerrata.h: SGX543 Core Revision unspecified"
		#endif
		#endif

		#define SGX_CORE_DEFINED
	#else
		#error "sgxerrata.h: SGX543 only supports MP configs (SGX_FEATURE_MP)"
	#endif
#endif

#if defined(SGX545) && !defined(SGX_CORE_DEFINED)
	/* define the _current_ SGX545 RTL head revision */
	#define SGX_CORE_REV_HEAD	0
	#if defined(USE_SGX_CORE_REV_HEAD)
		/* build config selects Core Revision to be the Head */
		#define SGX_CORE_REV	SGX_CORE_REV_HEAD
	#endif

	#if SGX_CORE_REV == 100
		#define FIX_HW_BRN_26620
		#define FIX_HW_BRN_27266
		#define FIX_HW_BRN_27456
	#else
	#if SGX_CORE_REV == 109

	#else
	#if SGX_CORE_REV == SGX_CORE_REV_HEAD
		/* RTL head - no BRNs to apply */
	#else
		#error "sgxerrata.h: SGX545 Core Revision unspecified"
	#endif
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

#endif /* _SGXERRATA_KM_H_ */

/******************************************************************************
 End of file (sgxerrata.h)
******************************************************************************/
