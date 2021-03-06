#include "sm_perlin_data.h"
#include "simd.h"
#include "surface.h"
#include "mesh.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef EXPORT
#define EXPORT
#endif

#include "inline.h"

/*original (i.e. reference) ken perlin noise 
* coherent noise function over 1, 2 or 3 dimensions */
/* (copyright Ken Perlin) */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static void init(void);

#define s_curve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( (a) + t * ((b) - (a)) )

#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.;

INLINE float noise3(float vec[3])
{
	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
	register i, j;

	setup(0, bx0, bx1, rx0, rx1);
	setup(1, by0, by1, ry0, ry1);
	setup(2, bz0, bz1, rz0, rz1);

	i = p[ bx0 ];
	j = p[ bx1 ];

	b00 = p[ i + by0 ];
	b10 = p[ j + by0 ];
	b01 = p[ i + by1 ];
	b11 = p[ j + by1 ];

	t  = s_curve(rx0);
	sy = s_curve(ry0);
	sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

	q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
	q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
	a = lerp(t, u, v);

	q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
	q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
	b = lerp(t, u, v);

	c = lerp(sy, a, b);

	q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
	q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
	a = lerp(t, u, v);

	q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
	q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
	b = lerp(t, u, v);

	d = lerp(sy, a, b);

	return lerp(sz, c, d);
}

EXPORT void sm_perlin(StackMachine* sm) {
	floatf vec[3];

	vec[0] = SPOP(sm);
	vec[1] = SPOP(sm);
	vec[2] = SPOP(sm);

	floatf ret = noise3(vec);

	SLOAD(sm, ret);
}

//XXX implement me properly!
EXPORT void sm_perlin_dv(StackMachine* sm) {
	floatf x = SPOP(sm);
	floatf y = SPOP(sm);
	floatf z = SPOP(sm);
	floatf dx = SPOP(sm);
	floatf dy = SPOP(sm);
	floatf dz = SPOP(sm);

	float a[3] = { x, y, z }, b[3] = { x + dx, y + dy, z + dz };
	
	floatf r1 = noise3(a);
	floatf r2 = noise3(b);

	floatf df = sqrt(dx * dx + dy * dy + dz * dz);
	floatf f = (r2 - r1) / (0.0000001f+df);
	//f = dv[0] * dx + dv[1] * dy + dv[2] * dz;

	SLOAD(sm, f);
}

//where the heck did I get this code from?
#if 0
static float grads[][3] = {
  {
    0.5416718730792324,
    0.8209693335149857,
    0.18055729102641088
  },
  {
    -0.15358394882501525,
    -0.8489030647229927,
    -0.5057425801405548
  },
  {
    0.5740719325325999,
    -0.5040327693334181,
    0.6452847307323888
  },
  {
    0.14095075243069152,
    0.7291915201134254,
    -0.6696361791181045
  },
  {
    0.8988914796715209,
    -0.16755360022655982,
    0.4048702246708958
  },
  {
    0.3261095765761692,
    0.9221317472098343,
    -0.20814798786692618
  },
  {
    0.4917016625376568,
    -0.3787615329485101,
    0.7840721753870614
  },
  {
    -0.10165524043927711,
    0.6394876658325922,
    -0.7620510070456018
  },
  {
    -0.6604283446498062,
    0.6886389548758343,
    -0.2993506161854954
  },
  {
    -0.5763443511294891,
    0.4211128527755858,
    0.7003507365230196
  },
  {
    0.26340726003044773,
    -0.37582940929896225,
    0.8884643326939159
  },
  {
    -0.23332410312511723,
    -0.8187542275582157,
    0.5245963951042832
  },
  {
    0.5864483306365353,
    0.7487254811272762,
    0.30901215090081796
  },
  {
    0.38086777548227113,
    0.9212355819238904,
    -0.0791501117910604
  },
  {
    0.45072446638546976,
    -0.8641448789727783,
    0.2238327133076304
  },
  {
    -0.2805257188392011,
    0.9489423451928392,
    0.1442697008025743
  },
  {
    0.8205525704404373,
    0.03693268152135386,
    -0.5703765915421439
  },
  {
    -0.011069011896917644,
    -0.5884255151142322,
    -0.8084756583461103
  },
  {
    -0.2701950221456542,
    0.22577623267805438,
    0.9359592634112953
  },
  {
    0.8437962019740027,
    -0.5099376481219065,
    -0.16724701660163177
  },
  {
    0.5430108200472756,
    -0.2969648261904987,
    0.7854623742212176
  },
  {
    -0.9568262384607108,
    -0.2859207903632521,
    -0.05227667769837842
  },
  {
    -0.33566713128762077,
    -0.7652147461813594,
    -0.5493395754901849
  },
  {
    -0.777642311448719,
    -0.5701283642386307,
    -0.2650020447756531
  },
  {
    -0.3322760838629237,
    -0.5001652010401013,
    -0.7996420297615893
  },
  {
    -0.10852692766465614,
    0.9379077817952197,
    -0.3294706342295791
  },
  {
    0.700300549380293,
    -0.2878338014510103,
    -0.6532463878812652
  },
  {
    0.8716156121059684,
    0.08044947898749384,
    0.4835432825133431
  },
  {
    -0.5857229481935314,
    -0.7512977412943042,
    0.3041057906149677
  },
  {
    -0.5912710108335142,
    0.7381037627682936,
    -0.32496373203051604
  },
  {
    -0.7081009172519469,
    0.4222493279553738,
    -0.5659492875056801
  },
  {
    0.5732493342780193,
    0.705973753374519,
    0.415916169793988
  },
  {
    0.20848860758249313,
    0.6347822769142627,
    0.7440322314415264
  },
  {
    -0.4351743772500551,
    -0.6173965579159698,
    -0.655320342777896
  },
  {
    0.5184749432445388,
    -0.7816859371766075,
    -0.34662779583856346
  },
  {
    0.8577420630388338,
    -0.050255291692504055,
    0.5116179814576367
  },
  {
    -0.12967095490943295,
    0.394696281176412,
    0.9096154622027851
  },
  {
    0.43169069951364775,
    0.6117171863703637,
    0.6629066479169923
  },
  {
    0.2409338220065283,
    -0.2390795647256685,
    -0.9406337518630253
  },
  {
    0.4252409188860303,
    -0.29596132182234397,
    -0.8553227793588429
  },
  {
    0.5483851387953962,
    0.7410250905085449,
    -0.38749910294755135
  },
  {
    0.46147831744844797,
    0.8866826587381988,
    -0.02883791285630678
  },
  {
    -0.15128397602202637,
    0.834120342272324,
    0.5304304037349933
  },
  {
    -0.7133246771866529,
    -0.41032870371317753,
    0.5681533770256239
  },
  {
    -0.5221226943405586,
    0.39152982108063317,
    0.7576887825876287
  },
  {
    0.22596002183615263,
    0.5588310608242213,
    -0.7979034490399705
  },
  {
    0.23026523021897857,
    -0.8517692796203848,
    -0.47060282409604326
  },
  {
    -0.7788821294913703,
    -0.09123689435307977,
    -0.620498555572689
  },
  {
    -0.7042201588285996,
    -0.5575006932438705,
    -0.43962136541804436
  },
  {
    -0.5168420485919137,
    0.6408912310479339,
    -0.5675673764172655
  },
  {
    0.7926059636439536,
    -0.4481082772206619,
    0.41349094099190375
  },
  {
    -0.021601859891203792,
    -0.4579546026472365,
    -0.8887130816880398
  },
  {
    0.8542209607932865,
    -0.06941333669218909,
    0.5152556053364693
  },
  {
    0.26355926344743036,
    -0.6180644821668938,
    -0.7406300092048779
  },
  {
    -0.2575924808652076,
    0.8164011470607183,
    0.5168513140929906
  },
  {
    0.4846032666848273,
    0.573205679370016,
    0.6607532996938816
  },
  {
    0.7944918091782632,
    0.41691810558939046,
    -0.44154508080195415
  },
  {
    0.5040374061744036,
    0.5462764500381945,
    -0.6689755849884563
  },
  {
    -0.1109379370749103,
    -0.7343974983784127,
    -0.6695917326947017
  },
  {
    -0.014982601326402473,
    -0.6506161962122997,
    -0.7592589063578593
  },
  {
    -0.4300679808342594,
    0.14067233043763003,
    0.891769492251439
  },
  {
    -0.28916442468124104,
    0.5285191673950774,
    -0.7981550132616977
  },
  {
    -0.6388833823017115,
    -0.2708206698686156,
    -0.7200584619255853
  },
  {
    0.9263728642554317,
    -0.30214883642996565,
    -0.22480969066124687
  }
};

#define GRAD_SIZE 64
#define ABS(a) ((a) < 0 ? 0-(a) : (a))
#define FABS(a) ((a) < 0.0f ? 0.0f-(a) : (a))
#define DIMEN 220

typedef struct _Vec3 {
  floatf x;
  floatf y;
  floatf z;
} _Vec3;

double grad(int hash, double x, double y, double z) {
	int h = hash & 15;
	double u = h < 8 ? x : y,
		v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

INLINE _Vec3 noisexyz(floatf x, floatf y, floatf z, int thread) {
  intf idx = (intf)(DIMEN* DIMEN * DIMEN *(x* DIMEN * DIMEN + y* DIMEN + z));
  
  //idx *= -((idx>0)*2 - 1);
#ifdef SIMD
  //idx *= (idx<0)*2 + 1;
  idx[0] *= (idx[0]>0)*2-1;
  idx[1] *= (idx[1]>0)*2-1;
  idx[2] *= (idx[2]>0)*2-1;
  idx[3] *= (idx[3]>0)*2-1;
  /*
  idx[0] = ABS(idx[0]);
  idx[1] = ABS(idx[1]);
  idx[2] = ABS(idx[2]);
  idx[3] = ABS(idx[3]);
  */
  idx[0] = idx[0] & (GRAD_SIZE-1);
  idx[1] = idx[1] & (GRAD_SIZE-1);
  idx[2] = idx[2] & (GRAD_SIZE-1);
  idx[3] = idx[3] & (GRAD_SIZE-1);
#else
  //idx *= (idx>0)*2-1;
  idx = FABS(idx);
  idx = idx & (GRAD_SIZE-1);
#endif
  //idx = idx & ((1<<30)-1);
  
#ifdef SIMD
  {
    //static floatf rets[8][128][3] __attribute__ ((aligned (16)));
    //static int cur[8]={0,};
    //floatf *ret = rets[thread][cur[thread]];
    _Vec3 ret;
    
    //cur[thread] = (cur[thread]+1) & 7;
    
    ret.x[0] = grads[idx[0]][0];
    ret.x[1] = grads[idx[1]][0];
    ret.x[2] = grads[idx[2]][0];
    ret.x[3] = grads[idx[3]][0];
    
    ret.y[0] = grads[idx[0]][1];
    ret.y[1] = grads[idx[1]][1];
    ret.y[2] = grads[idx[2]][1];
    ret.y[3] = grads[idx[3]][1];

    ret.z[0] = grads[idx[0]][2];
    ret.z[1] = grads[idx[1]][2];
    ret.z[2] = grads[idx[2]][2];
    ret.z[3] = grads[idx[3]][2];
    
    return ret;
  }
#else
  {
  _Vec3 ret;

  ret.x = grads[idx][0];
  ret.y = grads[idx][1];
  ret.z = grads[idx][2];
  
  return ret;
  }
#endif
}

EXPORT floatf pnoise13(floatf u, floatf v, floatf w, floatf fu, floatf fv, floatf fz, floatf sz, int thread) {
    _Vec3 g1 = noisexyz(fu, fv, fz, thread);
    _Vec3 g2 = noisexyz(fu, fv+1.0f, fz, thread);
    _Vec3 g3 = noisexyz(fu+ 1.0f, fv+ 1.0f, fz, thread);
    _Vec3 g4 = noisexyz(fu+ 1.0f, fv, fz, thread);

    _Vec3 g5 = noisexyz(fu,    fv,    fz+ 1.0f, thread);
    _Vec3 g6 = noisexyz(fu,    fv+ 1.0f, fz+ 1.0f, thread);
    _Vec3 g7 = noisexyz(fu+ 1.0f, fv+ 1.0f, fz+ 1.0f, thread);
    _Vec3 g8 = noisexyz(fu+ 1.0f, fv,    fz+ 1.0f, thread);
    
    floatf vm1 = v-1.0f, um1 = u-1.0f, wm1 = w-1.0f;
    floatf u3, v3, w3;
    
    floatf c1, c2, c3, c4, c5, c6, c7, c8;
    floatf wm12, w2, v2, um12, vm12, u2;
    /*
    on factor;
    off period;
    
    um1 := u - 1; vm1 := v - 1; wm1 := w - 1;
    
    comment: c1 :=   u*g1x +   v*g1y + w*g1z;
    comment: c2 :=   u*g2x + vm1*g2y + w*g2z;
    comment: c3 := um1*g3x + vm1*g3y + w*g3z;
    comment: c4 := um1*g4x +   v*g4y + w*g4z;
    
    comment: c5 :=   u*g5x +   v*g5y + wm1*g5z;
    comment: c6 :=   u*g6x + vm1*g6y + wm1*g6z;
    comment: c7 := um1*g7x + vm1*g7y + wm1*g7z;
    comment: c8 := um1*g8x +   v*g8y + wm1*g8z;
    
    u1 := u*u*u*(u*(u*6 - 15) + 10);
    v1 := v*v*v*(v*(v*6 - 15) + 10);
    w1 := w*w*w*(w*(w*6 - 15) + 10);
    
    r1 := c1 + (c2 - c1)*v1;
    r2 := c4 + (c3 - c4)*v1;
    ra := r1 + (r2 - r1)*u1;
    
    r1 := c5 + (c6 - c5)*v1;
    r2 := c8 + (c7 - c8)*v1;
    rb := r1 + (r2 - r1)*u1;
    
    r := ra + (rb - ra)*w1;
    */
    
    //*

	wm1 = w - 1.0f;
	um1 = u - 1.0f;
	vm1 = v - 1.0f;
	
	c1 =   u*g1.x +   v*g1.y + w*g1.z;
    c2 =   u*g2.x + vm1*g2.y + w*g2.z;
    c3 = um1*g3.x + vm1*g3.y + w*g3.z;
    c4 = um1*g4.x +   v*g4.y + w*g4.z;
    
    c5 =   u*g5.x +   v*g5.y + wm1*g5.z;
    c6 =   u*g6.x + vm1*g6.y + wm1*g6.z;
    c7 = um1*g7.x + vm1*g7.y + wm1*g7.z;
    c8 = um1*g8.x +   v*g8.y + wm1*g8.z;
	
    wm12=wm1*wm1, w2=w*w, v2=v*v, u2=u*u, u3=u2*u, v3=v2*v, w3=w2*w, um12=um1*um1, vm12=vm1*vm1;
#if 1

    //first-order smoothstepp
    /*
    return ((((2*w+1)*wm12*c3-(2*w-3)*c7*w2)*(2*v-3)*v2-((2*w+
      1)*wm12*c4-(2*w-3)*c8*w2)*(2*v+1)*(vm12))*(2*u-3)*u2
      +1+(((2*w+1)*wm12*c1-(2*w-3)*c5*w2)*(2*v+1)*vm12-((2
      *w+1)*wm12*c2-(2*w-3)*c6*w2)*(2*v-3)*v2)*(2*u+1)*um12)*0.5 - 0.5;
    //*/
    
    //second-order smoothstep
    ///*
    return -((((3*(2*v-5)*v+10)*(c1-c2)*v3-c1+(3*(2*v-5)*v+10)*(c3-
      c4)*v3+c4)*(3*(2*u-5)*u+10)*u3-((3*(2*v-5)*v+10)*(c1-c2)*
      v3-c1)-(((3*(2*v-5)*v+10)*(c5-c6)*v3-c5+(3*(2*v-5)*v+10)*(
      c7-c8)*v3+c8)*(3*(2*u-5)*u+10)*u3-((3*(2*v-5)*v+10)*(c5-c6
      )*v3-c5)))*(3*(2*w-5)*w+10)*w3-(((3*(2*v-5)*v+10)*(c1-c2)*
      v3-c1+(3*(2*v-5)*v+10)*(c3-c4)*v3+c4)*(3*(2*u-5)*u+10)*u3
      -((3*(2*v-5)*v+10)*(c1-c2)*v3-c1)));
    //*/
#else

    wm1=w-1;
    um1=u-1;
    vm1=v-1;
    
    wm12=wm1*wm1, w2=w*w, v2=v*v, u2=u*u, um12=um1*um1, vm12=vm1*vm1;
#define g1x g1.x
#define g1y g1.y
#define g1z g1.z

#define g2x g2.x
#define g2y g2.y
#define g2z g2.z

#define g3x g3.x
#define g3y g3.y
#define g3z g3.z

#define g4x g4.x
#define g4y g4.y
#define g4z g4.z

#define g5x g5.x
#define g5y g5.y
#define g5z g5.z

#define g6x g6.x
#define g6y g6.y
#define g6z g6.z

#define g7x g7.x
#define g7y g7.y
#define g7z g7.z

#define g8x g8.x
#define g8y g8.y
#define g8z g8.z

  return ((((wm1*g6z+g6x*u+vm1*g6y)*(2*w-3)*w2-(g2x*u+g2z*w+(v
        -1)*g2y)*(2*w+1)*wm12)*(2*v-3)*v2-((g5x*u+g5y*v+wm1*
        g5z)*(2*w-3)*w2-(2*w+1)*wm12*c1)*(2*v+1)*vm12)*(2*u+
        1)*um12+1+(((vm1*g3y+g3z*w+um1*g3x)*(2*w+1)*wm12-(
        vm1*g7y+wm1*g7z+um1*g7x)*(2*w-3)*w2)*(2*v-3)*v2+(((w
        -1)*g8z+g8y*v+um1*g8x)*(2*w-3)*w2-(g4y*v+g4z*w+um1*g4x)*
        (2*w+1)*wm12)*(2*v+1)*vm12)*(2*u-3)*u2)*0.5 - 0.5;
//*/
#endif
}

EXPORT floatf pnoise13_dv(floatf dv[3], floatf u, floatf v, floatf w, floatf fu, floatf fv, floatf fz, floatf sz, int thread) {
    _Vec3 g1 = noisexyz(fu, fv, fz, thread);
    _Vec3 g2 = noisexyz(fu, (fv+1.0), fz, thread);
    _Vec3 g3 = noisexyz((fu+1.0), (fv+1.0), fz, thread);
    _Vec3 g4 = noisexyz((fu+1.0), fv, fz, thread);

    fz += 1.0f;
    
    _Vec3 g5 = noisexyz(fu, fv, fz, thread);
    _Vec3 g6 = noisexyz(fu, (fv+1.0), fz, thread);
    _Vec3 g7 = noisexyz((fu+1.0), (fv+1.0), fz, thread);
    _Vec3 g8 = noisexyz((fu+1.0), fv*sz, fz, thread);

    floatf u2=u*u, v2=v*v, w2=w*w;

    floatf ans3=-((g1.z-g5.z-(g5.z-g6.z)*(2.0*v-3.0)*v2+(g1.z-g2.z)*(2.0*v-3.0)*v2-(
            g5.z-g8.z+(g7.z-g8.z)*(2.0*v-3.0)*v2+(g5.z-g6.z)*(2.0*v-3.0)*v2)*(2.0*u-3.0)
            *u2+(g1.z-g4.z+(g3.z-g4.z)*(2.0*v-3.0)*v2+(g1.z-g2.z)*(2.0*v-3.0)*v2)*
            (2.0*u-3.0)*u2)*(2.0*w-3.0)*w2+(g1.z-g4.z+(g3.z-g4.z)*(2.0*v-3.0)*v2+(
            g1.z-g2.z)*(2.0*v-3.0)*v2)*(2.0*u-3.0)*u2+(g1.z-g2.z)*(2.0*v-3.0)*v2+g1.z
            );
    floatf ans2=6.0*(g5.x*u+g5.y*v-g1.z*w-g1.y*v-g1.x*u+(w-1.0)*g5.z+(g5.y*v-g6.x*u+
            g5.x*u-(v-1.0)*g6.y+(g5.z-g6.z)*(w-1.0))*(2.0*v-3.0)*v2+(g2.x*u+g2.z*w-g1.z
            *w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(2.0*v-3.0)*v2-((u-1.0)*g8.x-g5.x*u-(g5.z-
            g8.z)*(w-1.0)-(g5.y-g8.y)*v-((v-1.0)*g7.y-g8.y*v+(g7.z-g8.z)*(w-1.0)+(g7.x-
            g8.x)*(u-1.0))*(2.0*v-3.0)*v2-(g5.y*v-g6.x*u+g5.x*u-(v-1.0)*g6.y+(g5.z-g6.z
            )*(w-1.0))*(2.0*v-3.0)*v2)*(2.0*u-3.0)*u2+(g4.y*v+g4.z*w-g1.z*w-g1.y*v-
            g1.x*u+(u-1.0)*g4.x+(g4.y*v+g4.z*w-g3.z*w-(v-1.0)*g3.y-(g3.x-g4.x)*(u-1.0))*
            (2.0*v-3.0)*v2+(g2.x*u+g2.z*w-g1.z*w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(2.0*v-3.0)
            *v2)*(2.0*u-3.0)*u2)*(w-1.0)*w+ans3;
    floatf ans1=-ans2;
    dv[2] = ans1*0.5;
    
    ans3=6.0*(g2.x*u+g2.z*w-g1.z*w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(v-1.0)*v-((g1.y-
      g2.y)*(2.0*v-3.0)*v2+g1.y);
      
    ans2=(6.0*(g2.x*u+g2.z*w-g1.z*w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(v-1.0)*v+6.0*(
      g5.y*v-g6.x*u+g5.x*u-(v-1.0)*g6.y+(g5.z-g6.z)*(w-1.0))*(v-1.0)*v-(g1.y-g5.y-
      (g5.y-g6.y)*(2.0*v-3.0)*v2+(g1.y-g2.y)*(2.0*v-3.0)*v2)+(6.0*((v-1.0)*g7.y-
      g8.y*v+(g7.z-g8.z)*(w-1.0)+(g7.x-g8.x)*(u-1.0))*(v-1.0)*v+g5.y-g8.y+(g7.y-
      g8.y)*(2.0*v-3.0)*v2+(g5.y-g6.y)*(2.0*v-3.0)*v2+6.0*(g5.y*v-g6.x*u+g5.x*u-
      (v-1.0)*g6.y+(g5.z-g6.z)*(w-1.0))*(v-1.0)*v)*(2.0*u-3.0)*u2+(6.0*(g2.x*u+g2.z
      *w-g1.z*w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(v-1.0)*v+6.0*(g4.y*v+g4.z*w-g3.z*w-(
      v-1.0)*g3.y-(g3.x-g4.x)*(u-1.0))*(v-1.0)*v-(g1.y-g4.y+(g3.y-g4.y)*(2.0*v-3.0)*v2
      +(g1.y-g2.y)*(2.0*v-3.0)*v2))*(2.0*u-3.0)*u2)*(2.0*w-3.0)*w2+(6.0*(
      g2.x*u+g2.z*w-g1.z*w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(v-1.0)*v+6.0*(g4.y*v+g4.z*
      w-g3.z*w-(v-1.0)*g3.y-(g3.x-g4.x)*(u-1.0))*(v-1.0)*v-(g1.y-g4.y+(g3.y-g4.y)*
      (2.0*v-3.0)*v2+(g1.y-g2.y)*(2.0*v-3.0)*v2))*(2.0*u-3.0)*u2+ans3;
      
    ans1=-ans2;
  dv[1] = ans1*0.5;

  ans3=-((g1.x-g4.x+(g3.x-g4.x)*(2.0*v-3.0)*v2+(g1.x-g2.x)*(2.0*v-3.0)*v2)*
      (2.0*u-3.0)*u2+(g1.x-g2.x)*(2.0*v-3.0)*v2+g1.x);
      
  ans2=(6.0*(g4.y*v+g4.z*w-g1.z*w-g1.y*v-g1.x*u+(u-1.0)*g4.x+(g4.y*v+g4.z*w-
      g3.z*w-(v-1.0)*g3.y-(g3.x-g4.x)*(u-1.0))*(2.0*v-3.0)*v2+(g2.x*u+g2.z*w-g1.z
      *w-g1.y*v-g1.x*u+(v-1.0)*g2.y)*(2.0*v-3.0)*v2)*(u-1.0)*u-(6.0*((u-1.0)*g8.x-
      g5.x*u-(g5.z-g8.z)*(w-1.0)-(g5.y-g8.y)*v-((v-1.0)*g7.y-g8.y*v+(g7.z-g8.z)*(
      w-1.0)+(g7.x-g8.x)*(u-1.0))*(2.0*v-3.0)*v2-(g5.y*v-g6.x*u+g5.x*u-(v-1.0)*
      g6.y+(g5.z-g6.z)*(w-1.0))*(2.0*v-3.0)*v2)*(u-1.0)*u+g1.x-g5.x-(g5.x-g6.x)*(
      2.0*v-3.0)*v2+(g1.x-g2.x)*(2.0*v-3.0)*v2-(g5.x-g8.x+(g7.x-g8.x)*(2.0*v-3.0)*
      v2+(g5.x-g6.x)*(2.0*v-3.0)*v2)*(2.0*u-3.0)*u2+(g1.x-g4.x+(g3.x-g4.x)*(
      2.0*v-3.0)*v2+(g1.x-g2.x)*(2.0*v-3.0)*v2)*(2.0*u-3.0)*u2))*(2.0*w-3.0)*w2
      +6.0*(g4.y*v+g4.z*w-g1.z*w-g1.y*v-g1.x*u+(u-1.0)*g4.x+(g4.y*v+g4.z*w-g3.z*
      w-(v-1.0)*g3.y-(g3.x-g4.x)*(u-1.0))*(2.0*v-3.0)*v2+(g2.x*u+g2.z*w-g1.z*w-
      g1.y*v-g1.x*u+(v-1.0)*g2.y)*(2.0*v-3.0)*v2)*(u-1.0)*u+ans3;
  ans1=-ans2;
  dv[0]=ans1*0.5;
  
  return 0;
}

EXPORT void sm_perlin(StackMachine *sm) {
  floatf x = SPOP(sm);
  floatf y = SPOP(sm);
  floatf z = SPOP(sm);

  floatf u = x, v = y, w=z, fu=fast_floor(u), fv=fast_floor(v), fw=fast_floor(w);
  floatf f;
  
  u -= fu; v -= fv; w -= fw;

  f = pnoise13(u, v, w, fu, fv, fw, 1.0, sm->threadnr);
  
  SLOAD(sm, f);
}

EXPORT void sm_perlin_dv(StackMachine *sm) {
  floatf x = SPOP(sm);
  floatf y = SPOP(sm);
  floatf z = SPOP(sm);
  floatf dx = SPOP(sm);
  floatf dy = SPOP(sm);
  floatf dz = SPOP(sm);
  floatf dv[3];
  
  floatf u = x, v = y, w=z, fu=fast_floor(u), fv=fast_floor(v), fw=fast_floor(w);
  floatf f;
  
  u -= fu; v -= fv; w -= fw;

  pnoise13_dv(dv, u, v, w, fu, fv, fw, 1.0, sm->threadnr);
  //VECMULF(dv, 1.0/sz);
  
  f = dv[0]*dx + dv[1]*dy + dv[2]*dz;
  
  SLOAD(sm, f);
}
#endif