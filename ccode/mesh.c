//
// Marching Cubes Example Program 
// by Cory Bloyd (corysama@yahoo.com)
//
// A simple, portable and complete implementation of the Marching Cubes
// and Marching Tetrahedrons algorithms in a single source file.
// There are many ways that this code could be made faster, but the 
// intent is for the code to be easy to understand.
//
// For a description of the algorithm go to
// http://astronomy.swin.edu.au/pbourke/modelling/polygonise/
//
// This code is public domain.
//

#include "thread.h"
#include "timer.h"

#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "string.h"

#include "simd.h"

#include "mesh.h"
#include "hashtable.h"
#include "vec.h"

#include "pthread.h"
 
typedef float Vector3f[3];

//These tables are used so that everything can be done in little loops that you can look at all at once
// rather than in pages and pages of unrolled code.

//a2fVertexOffset lists the positions, relative to vertex0, of each of the 8 vertices of a cube
static const float a2fVertexOffset[8][3] =
{
        {0.0, 0.0, 0.0},{1.0, 0.0, 0.0},{1.0, 1.0, 0.0},{0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},{1.0, 0.0, 1.0},{1.0, 1.0, 1.0},{0.0, 1.0, 1.0}
};

//a2iEdgeConnection lists the index of the endpoint vertices for each of the 12 edges of the cube
static const int a2iEdgeConnection[12][2] = 
{
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
};

//a2fEdgeDirection lists the direction vector (vertex1-vertex0) for each edge in the cube
static const float a2fEdgeDirection[12][3] =
{
        {1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
        {1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
        {0.0, 0.0, 1.0},{0.0, 0.0, 1.0},{ 0.0, 0.0, 1.0},{0.0,  0.0, 1.0}
};

//a2iTetrahedronEdgeConnection lists the index of the endpoint vertices for each of the 6 edges of the tetrahedron
static const int a2iTetrahedronEdgeConnection[6][2] =
{
        {0,1},  {1,2},  {2,0},  {0,3},  {1,3},  {2,3}
};

//a2iTetrahedronEdgeConnection lists the index of verticies from a cube 
// that made up each of the six tetrahedrons within the cube
static const int a2iTetrahedronsInACube[6][4] =
{
        {0,5,1,6},
        {0,1,2,6},
        {0,2,3,6},
        {0,3,7,6},
        {0,7,4,6},
        {0,4,5,6},
};

//fGetOffset finds the approximate point of intersection of the surface
// between two points with the values fValue1 and fValue2
float fGetOffset(float fValue1, float fValue2, float fValueDesired) {
  float fDelta = fValue2 - fValue1;
  
  if(fDelta == 0.0) {
    return 0.5;
  }
  
  return (fValueDesired - fValue1)/fDelta;
}

MYEXPORT void smooth_topology(floatf (*sample)(floatf,floatf,floatf,int), 
                     float *verts, int *tris, int totvert, 
                     int tottri, float cellsize[3], int thread) 
{
  struct {int tot; float co[3];} *vs = MEM_calloc(sizeof(*vs)*totvert);
  float co2[3], cmpvec[3];
  float max_move = sqrt(DOT(cellsize, cellsize))*0.15;
  int si, i, j, *tri=tris;
  
  memset(vs, 0, sizeof(*vs)*totvert);
  printf("smoothing topology\n");
  
  for (si=0; si<1; si++) {
    for (i=0; i<totvert; i++) {
      VECLOAD(vs[i].co, (verts+i*3));
      vs[i].tot = 1;
    }
    
    for (i=0; i<tottri; i++, tri += 3) {
      int v1 = tri[0], v2 = tri[1], v3 = tri[2];

      VECADD(vs[v1].co, vs[v1].co, (verts+v2*3));
      VECADD(vs[v1].co, vs[v1].co, (verts+v3*3));
      
      VECADD(vs[v2].co, vs[v2].co, (verts+v1*3));
      VECADD(vs[v2].co, vs[v2].co, (verts+v3*3));
      
      VECADD(vs[v3].co, vs[v3].co, (verts+v2*3));
      VECADD(vs[v3].co, vs[v3].co, (verts+v1*3));
      
      vs[v1].tot += 2;
      vs[v2].tot += 2;
      vs[v3].tot += 2;
    }

    for (i=0; i<totvert; i++) {
      if (vs[i].tot == 0) {
        continue;
      }
      
      VECMULF(vs[i].co, 1.0/(float)vs[i].tot);
      VECLOAD((verts+i*3), vs[i].co);
    }
  }
  
  //return;
  
#define DF  0.0005f
#define IDF (1.0f/DF)

  //project back onto surface
  //try to use inside/outside edge method
#if 1
  for (j=0; j<4; j++) {
    
#ifdef SIMD
    for (i=0; i<totvert-4; i += 4) {
      float *co1 = vs[i].co, *co2 = vs[i+1].co, *co3 = vs[i+2].co, *co4 = vs[i+3].co;
#else
    for (i=0; i<totvert; i++) {
      float *co1 = vs[i].co;
#endif
      floatf dv[3], f, d, f2, l, dx, dy, dz;
      
#ifdef SIMD
      floatf xco = {co1[0], co2[0], co3[0], co4[0]};
      floatf yco = {co1[1], co2[1], co3[1], co4[1]};
      floatf zco = {co1[2], co2[2], co3[2], co4[2]};
#else
      floatf xco = co1[0], yco = co1[1], zco = co1[2];
#endif

      if (vs[i].tot == 0) {
        continue;
      }
      
      f = sample(xco, yco, zco, thread);
      
      //if (f >= -0.00001f && f <= 0.00001f)
      //    continue;
        
      dv[0] = (sample(xco+DF, yco, zco, thread) - f) * IDF;
      dv[1] = (sample(xco, yco+DF, zco, thread) - f) * IDF;
      dv[2] = (sample(xco, yco, zco+DF, thread) - f) * IDF;
      
      d = dv[0]*dv[0] + dv[1]*dv[1] + dv[2]*dv[2];
      //if (d == 0.0)
      //  continue;
      
      f /= d+0.000001f;
      f *= -0.25f;
      
#ifdef SIMD      
      co1[0] = co1[0] + f[0]*dv[0][0];
      co1[1] = co1[1] + f[0]*dv[0][1];
      co1[2] = co1[2] + f[0]*dv[0][2];
      
      co2[0] = co2[0] + f[1]*dv[1][0];
      co2[1] = co2[1] + f[1]*dv[1][1];
      co2[2] = co2[2] + f[1]*dv[1][2];
      
      co3[0] = co3[0] + f[2]*dv[2][0];
      co3[1] = co3[1] + f[2]*dv[2][1];
      co3[2] = co3[2] + f[2]*dv[2][2];
      
      co4[0] = co4[0] + f[3]*dv[3][0];
      co4[1] = co4[1] + f[3]*dv[3][1];
      co4[2] = co4[2] + f[3]*dv[3][2];
#else
      co1[0] = co1[0] + f*dv[0];
      co1[1] = co1[1] + f*dv[1];
      co1[2] = co1[2] + f*dv[2];
#endif
      //printf("projecting! %.2f %.2f %.2f %.2f\n", f, dv[0], dv[1], dv[2]);
    }
  
    for (i=0; i<totvert-4; i += 4) {
      float *co1 = vs[i].co, *co2 = vs[i+1].co, *co3 = vs[i+2].co, *co4 = vs[i+3].co;
      float *vco1 = verts+i*3, *vco2 = verts+(i+1)*3, *vco3 = verts+(i+2)*3, *vco4 = verts+(i+3)*3;
      int k=0;
      
      for (k=0; k<4; k++) {
        float *co = vs[i+k].co;
        float *vco = verts + (i+k)*3;
        
        VECSUB(cmpvec, co, vco);
        float l = sqrt(DOT(cmpvec, cmpvec));
        if (l > max_move) {
          VECMULF(cmpvec, max_move/l);
          VECADD(co, cmpvec, vco);
        }
        
        VECLOAD(vco, co);
      }
    }
  }
#endif
  MEM_free(vs);
}

static float mul_m4_v3(float v[3], float mat[4][4]) {
  float x = v[0], y = v[1], z = v[2];
  
  v[0] = x*mat[0][0] + y*mat[1][0] + z*mat[2][0] + mat[3][0];
  v[1] = x*mat[0][1] + y*mat[1][1] + z*mat[2][1] + mat[3][1];
  v[2] = x*mat[0][2] + y*mat[1][2] + z*mat[2][2] + mat[3][2];
  
  return 0.0;
}

void vMarchCube1(float fX, float fY, float fZ, float afCubeValue[8], HashTable *ht, int **tris, 
                 floatf (*sample)(floatf,floatf,floatf,int), float cellsize[3], int thread);

typedef struct ThreadJob {
  float *verts;
  int   *tris;
  int totvert, tottri;
  float min[3], max[3], matrix[4][4];
  int threadnr, done, running, index;
  floatf (*sample)(floatf, floatf, floatf, int);
  int ocdepth;
} ThreadJob;

void *run_thread_job(void *thread_data) {
  ThreadJob *job = thread_data;
  
  printf("thread %d running. %d\n", job->index, job->threadnr);
  
//void polygonize(floatf (*sample)(floatf, floatf, floatf,int), float **vertout, int *totvert, int **triout, int *tottri,
///                float min[3], float max[3], int ocdepth, float matrix[4][4], int thread) 
  
  polygonize(job->sample, &job->verts, &job->totvert, &job->tris, &job->tottri, job->min, job->max, job->ocdepth, job->matrix, job->threadnr);
  
  //printf("tottri: %d, totvert: %d\n", job->tottri, job->totvert);
  
  job->done = 1;
  printf("thread %d done. %d\n", job->index, job->threadnr);
  
  return NULL;
}
    
MYEXPORT void merge_tiles(ThreadJob *tiles, int tottile, float **vertout, int *totvert, int **triout, int *tottri, int thread) {
  int i, j, k;
  int *tris = NULL;
  float *verts = NULL;
  HashTable *ht = ht_new();
  
  *vertout = NULL;
  *triout = NULL;
  *tottri = 0;
  *totvert = 0;
  
  for (i=0; i<tottile; i++) {
    ThreadJob *job = tiles + i;
    int *ts = job->tris;
    float *vs = job->verts;
    
    for (j=0; j<job->tottri; j++) {
      int v1 = ts[j*3], v2 = ts[j*3+1], v3=ts[j*3+2];
      
      v1 = ht_ensurevert(ht, vs + v1*3);
      v2 = ht_ensurevert(ht, vs + v2*3);
      v3 = ht_ensurevert(ht, vs + v3*3);
      
      V_APPEND(tris, v1);
      V_APPEND(tris, v2);
      V_APPEND(tris, v3);
    }
    
    *tottri += job->tottri;
    
    V_FREE(job->tris);
    V_FREE(job->verts);
  }
  
  verts = MEM_malloc(sizeof(float)*ht->used*3);
  
  for (i=0; i<ht->size; i++) {
    HashEntry *en = ht->table + i;
    
    if (en->key == KEY_UNUSED) {
      continue;
    }
    
    //printf("iterating\n");
    
    j = en->value*3;
    verts[j++] = en->vec[0];
    verts[j++] = en->vec[1];
    verts[j++] = en->vec[2];
  }
  
  *totvert = ht->used;
  *vertout = verts;
  *triout = tris;
  
  ht_free(ht);
}

MYEXPORT void threaded_polygonize(floatf (*sample)(floatf, floatf, floatf,int), float **vertout, float **intensity_out, 
                         int *totvert, int **triout, int *tottri,
                float min[3], float max[3], int ocdepth, float matrix[4][4], int thread) 
{
  //polygonize(sample, vertout, totvert, triout, tottri, min, max, ocdepth, matrix, thread);
  //printf("totv: %d %d\n", *totvert, *tottri);
  //return;
  
  ThreadJob *tiles = MEM_calloc(sizeof(ThreadJob)*512);
  pthread_t threads[MAXTHREAD];
  int TWID = 2;
  float tilesize[3] = {(max[0]-min[0]) / TWID, (max[1]-min[1])/TWID, (max[2]-min[2])/TWID};
  int i, j, k, tottile, totleft, open_thread_nrs[MAXTHREAD];
  ThreadJob sjob;
    
  *totvert = 0;
  *tottri = 0;
  
  for (i=0; i<MAXTHREAD; i++) {
    open_thread_nrs[i] = 1;
  }
  
  tottile = 0;
  for (i=0; i<TWID; i++) {
    for (j=0; j<TWID; j++) {
      for (k=0; k<TWID; k++) {
        memset(&sjob, 0, sizeof(sjob));
        
        sjob.index = tottile++;
        
        sjob.min[0] = min[0] + tilesize[0]*i;
        sjob.min[1] = min[1] + tilesize[1]*j;
        sjob.min[2] = min[1] + tilesize[2]*k;
        
        sjob.max[0] = min[0] + tilesize[0]*(i+1);
        sjob.max[1] = min[1] + tilesize[1]*(j+1);
        sjob.max[2] = min[1] + tilesize[2]*(k+1);
        
        memcpy(sjob.matrix, matrix, sizeof(float)*16);
        V_APPEND(tiles, sjob);
      }
    }
  }
  
  printf("tottile: %d\n", tottile);
  totleft = tottile;
  while (totleft > 0) {
    int totrunning = 0;
    totleft = 0;
    doSleep(30);

    for (i=0; i<tottile; i++) {
      if (!tiles[i].done) {
        totleft++;
      }
    }
    
    for (i=0; i<tottile; i++) {
      if (tiles[i].running)
          totrunning++;
    }
    
    //printf("totrunning: %d\n", totrunning);
    //printf("totleft:    %d\n", totleft);
    
    for (i=0; i<tottile; i++) {
      ThreadJob *job = tiles + i;
      int err;
      
      if (job->done && job->running) { //job just finished
        open_thread_nrs[job->threadnr] = 1;
        job->running = 0;
      }
      
      if (job->running || job->done) {
        continue;
      }
    
      if (totrunning >= MAXTHREAD) {
        continue;
      }
        
      //spawn job
      for (j=0; j<MAXTHREAD; j++) {
        if (open_thread_nrs[j]) {
          break;
        }
      }
      
      if (j == MAXTHREAD) {
        //fprintf(stderr, "Thread corruption!\n");
        continue;
      }
      
      job->running = 1;
      job->threadnr = j;
      job->sample = sample;
      job->ocdepth = ocdepth-1;
      open_thread_nrs[j] = 0;
      
      err = pthread_create(&threads[job->threadnr], NULL, run_thread_job, (void *)job);
      
      if (err) {
         printf("ERROR; return code from pthread_create() is %d\n", err);
         open_thread_nrs[j] = 1;
      }
    }
  }
  
  printf("merging tiles. . .\n");
  merge_tiles(tiles, tottile, vertout, totvert, triout, tottri, thread);

  /*
  printf("relaxing topology. . .\n");
  
  for (i=0; i<4; i++) {
    int grid;
    float cellsize[3];
    
    VECSUB(cellsize, max, min);
    grid = (int)ceil(pow((int)pow(8.0, ocdepth), 1.0/3.0));
    VECMULF(cellsize, 1.0/grid);
    
    smooth_topology(sample, *vertout, *triout, *totvert, *tottri, cellsize, thread);
  }
  //*/
  
  
  MEM_free(tiles);
}
  
MYEXPORT void polygonize(floatf (*sample)(floatf, floatf, floatf,int), float **vertout, int *totvert, int **triout, int *tottri,
                float min[3], float max[3], int ocdepth, float matrix[4][4], int thread) 
{
  HashTable *ht = ht_new();
  float *verts=NULL;
  float *samplegrid=NULL;
  int grid, *tris=NULL;
  float afCubeValue[8];
  float cellsize[3], p[3], cellsize_muld[3], queue[4][3];
  int i, j, k, iqueue[4], totqueue=0, gridplus1;
  HashEntry *en;
  
  VECSUB(cellsize, max, min);
  grid = (int)ceil(pow((int)pow(8.0, ocdepth), 1.0/3.0));
  VECMULF(cellsize, 1.0/grid);
  
  gridplus1 = grid+1;
  
  cellsize_muld[0] = matrix[0][0]*matrix[0][0] + matrix[1][0]*matrix[1][0] + matrix[2][0]*matrix[2][0];
  cellsize_muld[1] = matrix[0][0]*matrix[0][0] + matrix[1][0]*matrix[1][0] + matrix[2][0]*matrix[2][0];
  cellsize_muld[2] = matrix[0][0]*matrix[0][0] + matrix[1][0]*matrix[1][0] + matrix[2][0]*matrix[2][0];
  
  cellsize_muld[0] = 1.0f / sqrt(cellsize_muld[0]);
  cellsize_muld[1] = 1.0f / sqrt(cellsize_muld[1]);
  cellsize_muld[2] = 1.0f / sqrt(cellsize_muld[2]);
  VECMUL(cellsize_muld, cellsize_muld, cellsize);
  
  samplegrid = MEM_calloc(sizeof(float)*(grid+1)*(grid+1)*(grid+1));
  
#if 1
  for (i=0; i<=grid; i++) {
    for (j=0; j<=grid; j++) {
      for (k=0; k<=grid; k++) {
        p[0] = min[0] + cellsize[0]*(float)(i);
        p[1] = min[1] + cellsize[1]*(float)(j);
        p[2] = min[2] + cellsize[2]*(float)(k);
        
        mul_m4_v3(p, matrix);
        
        VECLOAD(queue[totqueue], p);
        
        iqueue[totqueue] = k*gridplus1*gridplus1 + (j*gridplus1 + i);
        totqueue++;
        
        if (totqueue == 4) {
#ifdef SIMD
          floatf xvs = {queue[0][0], queue[1][0], queue[2][0], queue[3][0]};
          floatf yvs = {queue[0][1], queue[1][1], queue[2][1], queue[3][1]};
          floatf zvs = {queue[0][2], queue[1][2], queue[2][2], queue[3][2]};
          floatf f;
          
          totqueue=0;
          f = sample(xvs, yvs, zvs, thread);
          
          samplegrid[iqueue[0]] = f[0];
          samplegrid[iqueue[1]] = f[1];
          samplegrid[iqueue[2]] = f[2];
          samplegrid[iqueue[3]] = f[3];
#else
          int l;
        
          for (l=0; l<4; l++) {
            floatf xvs = queue[l][0], yvs = queue[l][1], zvs = queue[l][2];
            floatf f = sample(xvs, yvs, zvs, thread);
          
            samplegrid[iqueue[l]] = f;
			totqueue = 0;
          }
#endif
        }
      }
    }
  }
#endif
  
  for (i=0; i<grid; i++) {
    for (j=0; j<grid; j++) {
      for (k=0; k<grid; k++) {
        
        p[0] = min[0] + cellsize[0]*(float)i;
        p[1] = min[1] + cellsize[1]*(float)j;
        p[2] = min[2] + cellsize[2]*(float)k;
        
        //mul_m4_v3(p, matrix);
        
        afCubeValue[0] = samplegrid[(k+0)*gridplus1*gridplus1 +     j*gridplus1 + i];
        afCubeValue[1] = samplegrid[(k+0)*gridplus1*gridplus1 +     j*gridplus1 + i+1];
        afCubeValue[2] = samplegrid[(k+0)*gridplus1*gridplus1 + (j+1)*gridplus1 + i+1];
        afCubeValue[3] = samplegrid[(k+0)*gridplus1*gridplus1 + (j+1)*gridplus1 + i];

        afCubeValue[4] = samplegrid[(k+1)*gridplus1*gridplus1 +     j*gridplus1 + i];
        afCubeValue[5] = samplegrid[(k+1)*gridplus1*gridplus1 +     j*gridplus1 + i+1];
        afCubeValue[6] = samplegrid[(k+1)*gridplus1*gridplus1 + (j+1)*gridplus1 + i+1];
        afCubeValue[7] = samplegrid[(k+1)*gridplus1*gridplus1 + (j+1)*gridplus1 + i];

        //printf("x y z: %.3f %.3f %.3f\n", x, y, z);
        //printf("%.2f %.2f %.2f %d %d %d\n", cellsize[0], cellsize[1], cellsize[2], i, j, k);
        vMarchCube1(p[0], p[1], p[2], afCubeValue, ht, &tris, sample, cellsize, thread);
        //vMarchCube1(p[0], p[1], p[2], ht, &tris, sample, cellsize, thread);
        
        //printf("marching\n");
      }
    }
  }
  
  //printf("done\n");
  
  if (ht->used == 0 || tris == NULL) { //we did nothing?
    *vertout = NULL;
    *triout = NULL;
    *totvert = *tottri = 0;
    
    MEM_free(samplegrid);
    ht_free(ht);
    return; 
  }
  
  V_RESIZE(verts, ht->used*3);
  
  *vertout = verts;
  *triout = tris;
  
  *totvert = ht->used;
  *tottri = V_COUNT(tris)/3;
    
  en = ht->table;
  for (i=0; i<ht->size; i++, en++) {
    if (en->key == KEY_UNUSED) {
      continue;
    }
    
    //printf("iterating\n");
    
    j = en->value*3;
    verts[j++] = en->vec[0];
    verts[j++] = en->vec[1];
    verts[j++] = en->vec[2];
  }
  
  printf("done\n");

  MEM_free(samplegrid);
  ht_free(ht);
}

//vMarchCube1 performs the Marching Cubes algorithm on a single cube
void vMarchCube1(float fX, float fY, float fZ, float afCubeValue[8], HashTable *ht, int **tris, 
                 floatf (*sample)(floatf,floatf,floatf,int), float cellsize[3], int thread)
{
  extern int aiCubeEdgeFlags[256];
  extern int a2iTriangleConnectionTable[256][16];

  int iCorner, iVertex, iVertexTest, iEdge, iTriangle, iFlagIndex, iEdgeFlags;
  float fOffset;
  float sColor[3];
  //float afCubeValue[8];
  float asEdgeVertex[12][3];
  float asEdgeNorm[12][3];

//  v4sf vs[][3] = {
//        {fX,             fY,             fZ},
//        {fX+cellsize[0], fY,             fZ},
//        {fX+cellsize[0], fY+cellsize[1], fZ},
//        {fX,             fY+cellsize[1], fZ}, 

//        {fX,             fY,             fZ+cellsize[2]},
//        {fX+cellsize[0], fY,             fZ+cellsize[2]},
//        {fX+cellsize[0], fY+cellsize[1], fZ+cellsize[2]}, 
//        {fX,             fY+cellsize[1], fZ+cellsize[2]}
//  };
/*
#if 0
#ifdef SIMD
  v4sf xvs = {fX, fX+cellsize[0], fX+cellsize[0], fX};
  v4sf yvs = {fY, fY, fY+cellsize[1], fY+cellsize[1]};
  
  v4sf ra, zvs = {fZ, fZ, fZ, fZ};
  
  ra = sample(xvs, yvs, zvs, thread);
  afCubeValue[0] = ra[0]; afCubeValue[1] = ra[1]; afCubeValue[2] = ra[2]; afCubeValue[3] = ra[3];

  //zvs += cellsize[2];
  zvs[0] += cellsize[2];
  zvs[1] += cellsize[2];
  zvs[2] += cellsize[2];
  zvs[3] += cellsize[2];
  
  ra = sample(xvs, yvs, zvs, thread);
  afCubeValue[4] = ra[0]; afCubeValue[5] = ra[1]; afCubeValue[6] = ra[2]; afCubeValue[7] = ra[3];
#else  
  //Make a local copy of the values at the cube's corners
  for(iVertex = 0; iVertex < 8; iVertex++) {
          afCubeValue[iVertex] = sample(fX + a2fVertexOffset[iVertex][0]*cellsize[0],
                                        fY + a2fVertexOffset[iVertex][1]*cellsize[1],
                                        fZ + a2fVertexOffset[iVertex][2]*cellsize[2], thread);
  }
#endif
#endif
*/
  //Find which vertices are inside of the surface and which are outside
  iFlagIndex = 0;
  for(iVertexTest = 0; iVertexTest < 8; iVertexTest++)
  {
    if(afCubeValue[iVertexTest] <= 0.0) 
      iFlagIndex |= 1<<iVertexTest;
  }

  //Find which edges are intersected by the surface
  iEdgeFlags = aiCubeEdgeFlags[iFlagIndex];

  //If the cube is entirely inside or outside of the surface, then there will be no intersections
  if(iEdgeFlags == 0) 
  {
    return;
  }

  //Find the point of intersection of the surface with each edge
  //Then find the normal to the surface at those points
  for(iEdge = 0; iEdge < 12; iEdge++) {
    //if there is an intersection on this edge
    if(iEdgeFlags & (1<<iEdge)) {
      fOffset = fGetOffset(afCubeValue[ a2iEdgeConnection[iEdge][0] ], 
                           afCubeValue[ a2iEdgeConnection[iEdge][1] ], 0.0);

      asEdgeVertex[iEdge][0] = fX + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][0]  +  
                               fOffset * a2fEdgeDirection[iEdge][0]) * cellsize[0];
      asEdgeVertex[iEdge][1] = fY + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][1]  +  
                               fOffset * a2fEdgeDirection[iEdge][1]) * cellsize[1];
      asEdgeVertex[iEdge][2] = fZ + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][2]  +  
                               fOffset * a2fEdgeDirection[iEdge][2]) * cellsize[2];
    }
  }

  //Draw the triangles that were found.  There can be up to five per cube
  for(iTriangle = 0; iTriangle < 5; iTriangle++) {
    int v1, v2, v3;

    if(a2iTriangleConnectionTable[iFlagIndex][3*iTriangle] < 0)
            break;
    
    v1 = a2iTriangleConnectionTable[iFlagIndex][3*iTriangle];
    v2 = a2iTriangleConnectionTable[iFlagIndex][3*iTriangle+1];
    v3 = a2iTriangleConnectionTable[iFlagIndex][3*iTriangle+2];
    
    //printf("add triangle...\n");
    
    v1 = ht_ensurevert(ht,  asEdgeVertex[v1]);
    v2 = ht_ensurevert(ht,  asEdgeVertex[v2]);
    v3 = ht_ensurevert(ht,  asEdgeVertex[v3]);
    
    //printf("append triangle... %d\n", ht->used);
    
    V_APPEND((*tris), v1);
    V_APPEND((*tris), v2);
    V_APPEND((*tris), v3);
  }
}

#if 0
//vMarchTetrahedron performs the Marching Tetrahedrons algorithm on a single tetrahedron
void vMarchTetrahedron(Vector3f *pasTetrahedronPosition, float *pafTetrahedronValue)
{
        extern int aiTetrahedronEdgeFlags[16];
        extern int a2iTetrahedronTriangles[16][7];

        int iEdge, iVert0, iVert1, iEdgeFlags, iTriangle, iCorner, iVertex, iFlagIndex = 0;
        float fOffset, fInvOffset, fValue = 0.0;
        Vector3f asEdgeVertex[6];
        Vector3f asEdgeNorm[6];
        Vector3f sColor;

        //Find which vertices are inside of the surface and which are outside
        for(iVertex = 0; iVertex < 4; iVertex++)
        {
                if(pafTetrahedronValue[iVertex] <= 0.0) 
                        iFlagIndex |= 1<<iVertex;
        }

        //Find which edges are intersected by the surface
        iEdgeFlags = aiTetrahedronEdgeFlags[iFlagIndex];

        //If the tetrahedron is entirely inside or outside of the surface, then there will be no intersections
        if(iEdgeFlags == 0)
        {
                return;
        }
        //Find the point of intersection of the surface with each edge
        // Then find the normal to the surface at those points
        for(iEdge = 0; iEdge < 6; iEdge++)
        {
                //if there is an intersection on this edge
                if(iEdgeFlags & (1<<iEdge))
                {
                        iVert0 = a2iTetrahedronEdgeConnection[iEdge][0];
                        iVert1 = a2iTetrahedronEdgeConnection[iEdge][1];
                        fOffset = fGetOffset(pafTetrahedronValue[iVert0], pafTetrahedronValue[iVert1], 0.0);
                        fInvOffset = 1.0 - fOffset;

                        asEdgeVertex[iEdge][0] = fInvOffset*pasTetrahedronPosition[iVert0][0]  +  fOffset*pasTetrahedronPosition[iVert1][0];
                        asEdgeVertex[iEdge][1] = fInvOffset*pasTetrahedronPosition[iVert0][1]  +  fOffset*pasTetrahedronPosition[iVert1][1];
                        asEdgeVertex[iEdge][2] = fInvOffset*pasTetrahedronPosition[iVert0][2]  +  fOffset*pasTetrahedronPosition[iVert1][2];
                        
                        vGetNormal(asEdgeNorm[iEdge], asEdgeVertex[iEdge][0], asEdgeVertex[iEdge][1], asEdgeVertex[iEdge][2]);
                }
        }
        //Draw the triangles that were found.  There can be up to 2 per tetrahedron
        for(iTriangle = 0; iTriangle < 2; iTriangle++)
        {
                if(a2iTetrahedronTriangles[iFlagIndex][3*iTriangle] < 0)
                        break;

                for(iCorner = 0; iCorner < 3; iCorner++)
                {
                        iVertex = a2iTetrahedronTriangles[iFlagIndex][3*iTriangle+iCorner];

                        vGetColor(sColor, asEdgeVertex[iVertex], asEdgeNorm[iVertex]);
                        glColor3f(sColor[0], sColor[1], sColor[2]);
                        glNormal3f(asEdgeNorm[iVertex][0],   asEdgeNorm[iVertex][1],   asEdgeNorm[iVertex][2]);
                        glVertex3f(asEdgeVertex[iVertex][0], asEdgeVertex[iVertex][1], asEdgeVertex[iVertex][2]);
                }
        }
}



//vMarchCube2 performs the Marching Tetrahedrons algorithm on a single cube by making six calls to vMarchTetrahedron
void vMarchCube2(float fX, float fY, float fZ)
{
        int iVertex, iTetrahedron, iVertexInACube;
        Vector3f asCubePosition[8];
        float  afCubeValue[8];
        Vector3f asTetrahedronPosition[4];
        float  afTetrahedronValue[4];

        //Make a local copy of the cube's corner positions
        for(iVertex = 0; iVertex < 8; iVertex++)
        {
                asCubePosition[iVertex][0] = fX + a2fVertexOffset[iVertex][0];
                asCubePosition[iVertex][1] = fY + a2fVertexOffset[iVertex][1];
                asCubePosition[iVertex][2] = fZ + a2fVertexOffset[iVertex][2];
        }

        //Make a local copy of the cube's corner values
        for(iVertex = 0; iVertex < 8; iVertex++)
        {
                afCubeValue[iVertex] = fSample(asCubePosition[iVertex][0],
                                                   asCubePosition[iVertex][1],
                                               asCubePosition[iVertex][2]);
        }

        for(iTetrahedron = 0; iTetrahedron < 6; iTetrahedron++)
        {
                for(iVertex = 0; iVertex < 4; iVertex++)
                {
                        iVertexInACube = a2iTetrahedronsInACube[iTetrahedron][iVertex];
                        asTetrahedronPosition[iVertex][0] = asCubePosition[iVertexInACube][0];
                        asTetrahedronPosition[iVertex][1] = asCubePosition[iVertexInACube][1];
                        asTetrahedronPosition[iVertex][2] = asCubePosition[iVertexInACube][2];
                        afTetrahedronValue[iVertex] = afCubeValue[iVertexInACube];
                }
                vMarchTetrahedron(asTetrahedronPosition, afTetrahedronValue);
        }
}
        

//vMarchingCubes iterates over the entire dataset, calling vMarchCube on each cube
void vMarchingCubes()
{
        int iX, iY, iZ;
        for(iX = 0; iX < iDataSetSize; iX++)
        for(iY = 0; iY < iDataSetSize; iY++)
        for(iZ = 0; iZ < iDataSetSize; iZ++)
        {
                vMarchCube(iX*fStepSize, iY*fStepSize, iZ*fStepSize, fStepSize);
        }
}
#endif

// For any edge, if one vertex is inside of the surface and the other is outside of the surface
//  then the edge intersects the surface
// For each of the 4 vertices of the tetrahedron can be two possible states : either inside or outside of the surface
// For any tetrahedron the are 2^4=16 possible sets of vertex states
// This table lists the edges intersected by the surface for all 16 possible vertex states
// There are 6 edges.  For each entry in the table, if edge #n is intersected, then bit #n is set to 1

int aiTetrahedronEdgeFlags[16]=
{
        0x00, 0x0d, 0x13, 0x1e, 0x26, 0x2b, 0x35, 0x38, 0x38, 0x35, 0x2b, 0x26, 0x1e, 0x13, 0x0d, 0x00, 
};


// For each of the possible vertex states listed in aiTetrahedronEdgeFlags there is a specific triangulation
// of the edge intersection points.  a2iTetrahedronTriangles lists all of them in the form of
// 0-2 edge triples with the list terminated by the invalid value -1.
//
// I generated this table by hand

int a2iTetrahedronTriangles[16][7] =
{
        {-1, -1, -1, -1, -1, -1, -1},
        { 0,  3,  2, -1, -1, -1, -1},
        { 0,  1,  4, -1, -1, -1, -1},
        { 1,  4,  2,  2,  4,  3, -1},

        { 1,  2,  5, -1, -1, -1, -1},
        { 0,  3,  5,  0,  5,  1, -1},
        { 0,  2,  5,  0,  5,  4, -1},
        { 5,  4,  3, -1, -1, -1, -1},

        { 3,  4,  5, -1, -1, -1, -1},
        { 4,  5,  0,  5,  2,  0, -1},
        { 1,  5,  0,  5,  3,  0, -1},
        { 5,  2,  1, -1, -1, -1, -1},

        { 3,  4,  2,  2,  4,  1, -1},
        { 4,  1,  0, -1, -1, -1, -1},
        { 2,  3,  0, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1},
};

// For any edge, if one vertex is inside of the surface and the other is outside of the surface
//  then the edge intersects the surface
// For each of the 8 vertices of the cube can be two possible states : either inside or outside of the surface
// For any cube the are 2^8=256 possible sets of vertex states
// This table lists the edges intersected by the surface for all 256 possible vertex states
// There are 12 edges.  For each entry in the table, if edge #n is intersected, then bit #n is set to 1

int aiCubeEdgeFlags[256]=
{
        0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00, 
        0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 
        0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30, 
        0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 
        0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60, 
        0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 
        0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950, 
        0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 
        0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0, 
        0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650, 
        0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0, 
        0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460, 
        0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0, 
        0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230, 
        0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190, 
        0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
};

//  For each of the possible vertex states listed in aiCubeEdgeFlags there is a specific triangulation
//  of the edge intersection points.  a2iTriangleConnectionTable lists all of them in the form of
//  0-5 edge triples with the list terminated by the invalid value -1.
//  For example: a2iTriangleConnectionTable[3] list the 2 triangles formed when corner[0] 
//  and corner[1] are inside of the surface, but the rest of the cube is not.
//
//  I found this table in an example program someone wrote long ago.  It was probably generated by hand

int a2iTriangleConnectionTable[256][16] =  
{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
        {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
        {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
        {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
        {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
        {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
        {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
        {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
        {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
        {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
        {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
        {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
        {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
        {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
        {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
        {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
        {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
        {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
        {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
        {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
        {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
        {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
        {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
        {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
        {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
        {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
        {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
        {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
        {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
        {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
        {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
        {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
        {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
        {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
        {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
        {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
        {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
        {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
        {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
        {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
        {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
        {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
        {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
        {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
        {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
        {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
        {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
        {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
        {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
        {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
        {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
        {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
        {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
        {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
        {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
        {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
        {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
        {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
        {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
        {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
        {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
        {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
        {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
        {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
        {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
        {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
        {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
        {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
        {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
        {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
        {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
        {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
        {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
        {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
        {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
        {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
        {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
        {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
        {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
        {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
        {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
        {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
        {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
        {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
        {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
        {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
        {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
        {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
        {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
        {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
        {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
        {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
        {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
        {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
        {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
        {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
        {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
        {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
        {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
        {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
        {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
        {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
        {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
        {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
        {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
        {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
        {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
        {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
        {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
        {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
        {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
        {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
        {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
        {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
        {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
        {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
        {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
        {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
        {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
        {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
        {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
        {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
        {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
        {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
        {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
        {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
        {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
        {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
        {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
        {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
        {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
        {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
        {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
        {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
        {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
        {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
        {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
        {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
        {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
        {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
        {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
        {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
        {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
        {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
        {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
        {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
        {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
        {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
        {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
        {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
        {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
        {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
        {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
        {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
        {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
        {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};