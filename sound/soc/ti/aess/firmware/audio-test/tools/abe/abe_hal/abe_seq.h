/*
 * ALSA SoC OMAP ABE driver
 *
 * Author:	Laurent Le Faucheur <l-le-faucheur@ti.com>
 * 		Liam Girdwood <lrg@slimlogic.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
void abe_init_subroutine_table (void);
/*
 * Register Programming Examples
 *
 * 1. Power on sequence
 *
 * The modules HSLDO, NCP, LSLDO, LPPLL are enabled/disabled automatically by
 * the TWL6040 power state machine after pin AUDPWRON transitions from 0 ' 1.
 * No register writes are necessary.
 *
 * For the purposes of test it is possible to bypass the power state machine
 * and manually enable these modules in the same order as described in Fig 2-XX.
 * This can be done after VIO comes up and I2C register writes are possible.
 *
 * The manual sequence could be as follows
 * LDOCTL = 0x04 (Enable HSLDO)
 * NCPCTL = 0x03 (Enable NCP in auto mode)
 * LDOCTL = 0x05 (Enable LSLDO)
 * LPPLLCTL = 0x09 (Enable LPPLL with output frequency = 19.2MHz)
 *
 * Please see Fig 2-64 for details on details to be maintained between successive
 * I2C register writes.
 *
 * Further if the system MCLK is active the HPPLL could be enabled instead of the
 * LPPLL.
 * (a) For a square wave where slicer is not required
 * HPPLLCTL = 0x11 (Select HPPLL output, Enable HPPLL)
 * (a) For a sine wave where slicer is required
 * HPPLLCTL = 0x19 (Select HPPLL output, Enable Slicer, Enable HPPLL)
 *
 */
/*
 * 2. Setting up a stereo UPLINK path through MICAMPL, MICAMPR input amplifiers
 * AMICBCTL = 0x10
 * MICGAIN = 0x0F (Gain to 24 dB for L and R)
 * HPPLLCTL = 0x19 (Select HPPLL output, Enable Slicer, Enable HPPLL)
 * MICLCTL = 0x0D (Select MMIC input, Enable ADC)
 * MICRCTL = 0x0D (Select SMIC input, Enable ADC)
 *
 */
/*
 * 3. Setting up a stereo headset MP3 playback DNLINK path
 * Please see section 2.3.1.1 for details
 *
 * (b) HP
 * HSGAIN = 0x22 (-4 dB gain on L and R amplifiers)
 * HSLCTL = 0x01 (Enable HSDAC L, HP mode)
 * HSRCTL = 0x01 (Enable HSDAC R, HP mode)
 * Wait 80us
 * HSLCTL = 0x05 (Enable HSLDRV, HP mode)
 * HSRCTL = 0x05 (Enable HSRDRV, HP mode)
 * Wait 2ms
 * HSLCTL = 0x25 (Close HSDACL switch)
 * HSRCTL = 0x25 (Close HSDACR switch)
 *
 */
/*
 * (a) LP
 * HSGAIN = 0x22 (-4 dB gain on L and R amplifiers)
 * HSLCTL = 0x03 (Enable HSDAC L, LP mode)
 * HSRCTL = 0x03 (Enable HSDAC R, LP mode)
 * Wait 80us
 * HSLCTL = 0x0F (Enable HSLDRV, LP mode)
 * HSRCTL = 0x0F (Enable HSRDRV, LP mode)
 * Wait 2ms
 * HSLCTL = 0x2F (Close HSDACL switch)
 * HSRCTL = 0x2F (Close HSDACR switch)
 *
 */
/*
 * 4. Setting up a stereo FM playback path on headset
 * (a) HP
 * LINEGAIN = 0x1B (0dB gain on L and R inputs)
 * MICLCTL = 0x02 (Enable Left LINEAMP)
 * MICRCTL = 0x02 (Enable Right LINEAMP)
 * HSGAIN = 0x22 (-4 dB gain on L and R amplifiers)
 * HSLCTL = 0x04 (Enable HSLDRV in HP mode)
 * HSRCTL = 0x04 (Enable HSRDRV in HP mode)
 * Wait 2ms
 * HSLCTL = 0x44 (Close FMLOOP switch)
 * HSRCTL = 0x44 (Close FMLOOP switch)
 *
 *
 */
/*
 * (b) LP
 * LINEGAIN = 0x1B (0dB gain on L and R inputs)
 * MICLCTL = 0x02 (Enable Left LINEAMP)
 * MICRCTL = 0x02 (Enable Right LINEAMP)
 * HSGAIN = 0x22 (-4 dB gain on L and R amplifiers)
 * HSLCTL = 0x0C (Enable HSLDRV in LP mode)
 * HSRCTL = 0x0C (Enable HSRDRV in LP mode)
 * Wait 2ms
 * HSLCTL = 0x4C (Close FMLOOP switch)
 * HSRCTL = 0x4C (Close FMLOOP switch)
 *
 */
/*
 * 5. Setting up a handset call
 *
 * UPLINK
 *
 * AMICBCTL = 0x10
 * MICGAIN = 0x0F (Gain to 24 dB for L and R)
 * HPPLLCTL = 0x19 (Select HPPLL output, Enable Slicer, Enable HPPLL)
 * MICLCTL = 0x0D (Select MMIC input, Enable ADC)
 * MICRCTL = 0x0D (Select SMIC input, Enable ADC)
 *
 * DNLINK
 *
 * HSLCTL = 0x01 (Enable HSDACL, HP mode)
 * Wait 80us
 * EARCTL = 0x03 (Enable EAR, Gain = min, by default enabling EAR connects
 * HSDACL output to EAR)
 *
 */
