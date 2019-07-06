#ifndef _CURVE_H
#define _CURVE_H

typedef struct CurvePoint {
  float co[3];
  float width;
  float twist;
  int  flag;
} CurvePoint;

typedef struct BezierSpline {
  CurvePoint *points;
  int totpoint;
  int flag;
} BezierSpline;

//curvepoint->flag
#define SELECT     1
#define NEW_PATH   2
#define CLOSE_PATH 4

#endif /* _CURVE_H */
