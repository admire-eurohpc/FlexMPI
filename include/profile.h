/**
* @version		FlexMPI v3.1
* @copyright	Copyright (C) 2018 Universidad Carlos III de Madrid. All rights reserved.
* @license		GNU/GPL, see LICENSE.txt
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You have received a copy of the GNU General Public License in LICENSE.txt
* also available in <http://www.gnu.org/licenses/gpl.html>.
*
* See COPYRIGHT.txt for copyright notices and details.
*/

/****************************************************************************************************************************************
 *																																		*
 *	FLEX-MPI																															*
 *																																		*
 *	File:       profile.h																													*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EMPI_PROFILE_H_
#define _EMPI_PROFILE_H_

/* profiling data */

//1M
static const long long EMPI_GLOBAL_profile_flops [10] = {
50932863,
1970121006,
40408931587,
269588277536,
159257489501,
34228349115,
7350318524,
1534344389,
286209488,
65675418
};

//1M
static const double EMPI_GLOBAL_profile_mflops [10] = {
33.97853,
787.87124,
863.36904,
888.94169,
869.32672,
629.22612,
606.53868,
483.56276,
162.03013,
44.70303
};

//1M
static const double EMPI_GLOBAL_profile_comms [10] = {
0.86371,
3.56635,
12.62055,
50.80306,
26.99275,
11.61978,
3.57941,
1.36330,
1.06870,
1.86650
};

//1M
static const double EMPI_GLOBAL_profile_weigth [10] = {
0.00326,
0.00837,
0.08202,
0.48869,
0.29010,
0.09112,
0.02167,
0.00626,
0.00391,
0.00460
};

#endif
