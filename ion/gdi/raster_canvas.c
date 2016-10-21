#include "../ion.h"
#include "../bsp.h"
#include "gdi.h"
#include <math.h>
#include <string.h>

typedef int out_code_t;

static const int INSIDE = 0; // 0000
static const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)

// ASSUME THAT xmax, xmin, ymax and ymin are global constants.

inline bool point_in_rect(const point_t *pt, const rect_t *rect)
  {
  return pt->x >= rect->left &&
    pt->y >= rect->top &&
    pt->x < rect->right &&
    pt->y < rect->bottom;
  }

inline point_t *top_left(const rect_t *rect, point_t *pt)
  {
  pt->x = rect->left;
  pt->y = rect->top;
  
  return pt;
  }

inline point_t *top_right(const rect_t *rect, point_t *pt)
  {
  pt->x = rect->right;
  pt->y = rect->top;
  
  return pt;
  }

inline point_t *bottom_left(const rect_t *rect, point_t *pt)
  {
  pt->x = rect->left;
  pt->y = rect->bottom;
  
  return pt;
  }

inline point_t *bottom_right(const rect_t *rect, point_t *pt)
  {
  pt->x = rect->right;
  pt->y = rect->bottom;
  
  return pt;
  }

inline void normalize(rect_t *rect)
  {
  gdi_dim_t tmp;
  if(rect->left > rect->right)
    {
    tmp = rect->left;
    rect->left = rect->right;
    rect->right = tmp;
    }

  if(rect->top > rect->bottom)
    {
    tmp = rect->top;
    rect->top = rect->bottom;
    rect->bottom = tmp;
    }
  }

inline gdi_dim_t rect_width(const rect_t *rect)
  {
  gdi_dim_t result;
  
  result = rect->right - rect->left;
  if(result < 0)
    return -result;
  
  return result;
  }

inline gdi_dim_t rect_height(const rect_t *rect)
  {
  gdi_dim_t result;
  
  result = rect->bottom - rect->top;
  if(result < 0)
    return -result;
  
  return result;
  }

inline rect_t *copy_rect(const rect_t *from, rect_t *to)
  {
  to->left = from->left;
  to->top = from->top;
  to->bottom = from->bottom;
  to->right = from->right;    
  
  return to;
  }

inline rect_t *make_rect(const point_t *pt, const extent_t *ex, rect_t *rect)
  {
  rect->left = pt->x;
  rect->right = pt->x + ex->dx;
  rect->top = pt->y;
  rect->bottom = pt->y + ex->dy;
  
  return rect;
  }

inline rect_t *offset_rect(const extent_t *ex, rect_t *rect)
  {
  rect->left += ex->dx;
  rect->right += ex->dx;
  rect->top += ex->dy;
  rect->bottom += ex->dy;
  
  return rect;
  }

inline bool is_equal(const point_t *p1, const point_t *p2)
  {
  return p1->x == p2->x && p1->y == p2->y;
  }

inline point_t *make_point(gdi_dim_t x, gdi_dim_t y, point_t *pt)
  {
  pt->x = x;
  pt->y = y;
  
  return pt;
  }

inline point_t *copy_point(const point_t *p1, point_t *p2)
  {
  p2->x = p1->x;
  p2->y = p1->y;
  
  return p2;
  }

inline point_t *swap_points(point_t *p1, point_t *p2)
  {
  gdi_dim_t td;
  size_t ts;

  td = p1->x;
  p1->x = p2->x;
  p2->x = td;
  td = p1->y;
  p1->y = p2->y;
  p2->y = td;
  
  return p1;
  }

inline rect_t *assign_rect(const rect_t *from, rect_t *to)
  {
  to->left = from->left;
  to->top = from->top;
  to->right = from->right;
  to->bottom = from->bottom;
  return to;
  }

inline rect_t *intersect_rect(const rect_t *from, rect_t *to)
  {
  to->left = max(to->left, from->left);
  to->top = max(to->top, from->top);
  to->right = min(to->right, from->right);
  to->bottom = min(to->bottom, from->bottom);
  
  if (to->left >= to->right || to->top >= to->bottom)
    to->left = to->right = to->top = to->bottom = 0;

  return to;
  }

inline void rect_extents(const rect_t *from, extent_t *ex)
  {
  ex->dx = from->right - from->left;
  ex->dy = from->bottom - from->top;
  }

static const float rotn_table[90][2] = {
{ 0.000000000, 1.000000000 },
{ 0.017452406, 0.999847695 },
{ 0.034899497, 0.999390827 },
{ 0.052335956, 0.998629535 },
{ 0.069756474, 0.997564050 },
{ 0.087155743, 0.996194698 },
{ 0.104528463, 0.994521895 },
{ 0.121869343, 0.992546152 },
{ 0.139173101, 0.990268069 },
{ 0.156434465, 0.987688341 },
{ 0.173648178, 0.984807753 },
{ 0.190808995, 0.981627183 },
{ 0.207911691, 0.978147601 },
{ 0.224951054, 0.974370065 },
{ 0.241921896, 0.970295726 },
{ 0.258819045, 0.965925826 },
{ 0.275637356, 0.961261696 },
{ 0.292371705, 0.956304756 },
{ 0.309016994, 0.951056516 },
{ 0.325568154, 0.945518576 },
{ 0.342020143, 0.939692621 },
{ 0.358367950, 0.933580426 },
{ 0.374606593, 0.927183855 },
{ 0.390731128, 0.920504853 },
{ 0.406736643, 0.913545458 },
{ 0.422618262, 0.906307787 },
{ 0.438371147, 0.898794046 },
{ 0.453990500, 0.891006524 },
{ 0.469471563, 0.882947593 },
{ 0.484809620, 0.874619707 },
{ 0.500000000, 0.866025404 },
{ 0.515038075, 0.857167301 },
{ 0.529919264, 0.848048096 },
{ 0.544639035, 0.838670568 },
{ 0.559192903, 0.829037573 },
{ 0.573576436, 0.819152044 },
{ 0.587785252, 0.809016994 },
{ 0.601815023, 0.798635510 },
{ 0.615661475, 0.788010754 },
{ 0.629320391, 0.777145961 },
{ 0.642787610, 0.766044443 },
{ 0.656059029, 0.754709580 },
{ 0.669130606, 0.743144825 },
{ 0.681998360, 0.731353702 },
{ 0.694658370, 0.719339800 },
{ 0.707106781, 0.707106781 },
{ 0.719339800, 0.694658370 },
{ 0.731353702, 0.681998360 },
{ 0.743144825, 0.669130606 },
{ 0.754709580, 0.656059029 },
{ 0.766044443, 0.642787610 },
{ 0.777145961, 0.629320391 },
{ 0.788010754, 0.615661475 },
{ 0.798635510, 0.601815023 },
{ 0.809016994, 0.587785252 },
{ 0.819152044, 0.573576436 },
{ 0.829037573, 0.559192903 },
{ 0.838670568, 0.544639035 },
{ 0.848048096, 0.529919264 },
{ 0.857167301, 0.515038075 },
{ 0.866025404, 0.500000000 },
{ 0.874619707, 0.484809620 },
{ 0.882947593, 0.469471563 },
{ 0.891006524, 0.453990500 },
{ 0.898794046, 0.438371147 },
{ 0.906307787, 0.422618262 },
{ 0.913545458, 0.406736643 },
{ 0.920504853, 0.390731128 },
{ 0.927183855, 0.374606593 },
{ 0.933580426, 0.358367950 },
{ 0.939692621, 0.342020143 },
{ 0.945518576, 0.325568154 },
{ 0.951056516, 0.309016994 },
{ 0.956304756, 0.292371705 },
{ 0.961261696, 0.275637356 },
{ 0.965925826, 0.258819045 },
{ 0.970295726, 0.241921896 },
{ 0.974370065, 0.224951054 },
{ 0.978147601, 0.207911691 },
{ 0.981627183, 0.190808995 },
{ 0.984807753, 0.173648178 },
{ 0.987688341, 0.156434465 },
{ 0.990268069, 0.139173101 },
{ 0.992546152, 0.121869343 },
{ 0.994521895, 0.104528463 },
{ 0.996194698, 0.087155743 },
{ 0.997564050, 0.069756474 },
{ 0.998629535, 0.052335956 },
{ 0.999390827, 0.034899497 },
{ 0.999847695, 0.017452406 },
  };

typedef struct _matrix_t {
  float s0, s1;
  float c0, c1;
  } matrix_t;
  
// first 2 are sin conversions
// second are cos conversion
static const matrix_t quadrants[4] = {
  {  1.0,  0.0,  0.0,  1.0 },     // 0..89
  {  0.0,  1.0, -1.0,  0.0 },     // 90..179
  { -1.0,  0.0,  0.0, -1.0 },     // 180..269
  {  0.0, -1.0,  1.0,  0.0 }
  };

inline float roundf(float value)
  {
  int val = value;
  float rem = value - val;
  if(fabs(rem) >= 0.5f)
    val = val < 0 ? val-1 : val+1;
  
  return val;
  }

// angle is in degrees, not radians.
static void rotate_point(const point_t *center, point_t *pt, int angle)
  {
  int la = angle % 90;
  int qd = angle / 90;
  // convert the angle to a usefull form
  float cos_theta = rotn_table[la][0] * quadrants[qd].c0  +
                    rotn_table[la][1] * quadrants[qd].c1;
  float sin_theta = rotn_table[la][0] * quadrants[qd].s0  +
                    rotn_table[la][1] * quadrants[qd].s1;

  // calc the transformation
  gdi_dim_t x2 = (gdi_dim_t)(roundf((pt->x - center->x) * cos_theta - (pt->y - center->y) * sin_theta));
  gdi_dim_t y2 = (gdi_dim_t)(roundf((pt->x - center->x) * sin_theta + (pt->y - center->y) * cos_theta));

  pt->x = x2 + center->x;
  pt->y = y2 + center->y;
 }


out_code_t compute_out_code(const point_t *pt, const rect_t *rect)
  {
  out_code_t code;

  code = INSIDE;          // initialised as being inside of clip window

  if(pt->x < rect->left)           // to the left of clip window
    code |= LEFT;
  else if(pt->x > rect->right)      // to the right of clip window
    code |= RIGHT;
  if(pt->y < rect->top)           // below the clip window
    code |= BOTTOM;
  else if(pt->y > rect->bottom)      // above the clip window
    code |= TOP;

  return code;
  }

// clip a line to within the clipping rectangle.  Return true if part of line is within the rectangle
static bool clip_line(point_t *p1, point_t *p2, const rect_t *clip_rect)
  {
  // compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
  out_code_t outcode0 = compute_out_code(p1, clip_rect);
  out_code_t outcode1 = compute_out_code(p2, clip_rect);

  while(true)
    {
    if(!(outcode0 | outcode1))
      { // Bitwise OR is 0. Trivially accept and get out of loop
      return true;        // line inside rectange
      }
    else if(outcode0 & outcode1)
      { // Bitwise AND is not 0. Trivially reject and get out of loop
      return false;       // line outside rectangle
      }
    else
      {
      // failed both tests, so calculate the line segment to clip
      // from an outside point to an intersection with clip edge
      float x, y;

      // At least one endpoint is outside the clip rectangle; pick it.
      out_code_t outcodeOut = outcode0 ? outcode0 : outcode1;

      // Now find the intersection point;
      // use formulas y = y0 + slope * (x - x0), x = x0 + (1 / slope) * (y - y0)
      if(outcodeOut & TOP)
        {           // point is above the clip rectangle
        x = p1->x + (p2->x - p1->x) * (clip_rect->bottom - p1->y) / (p2->y - p1->y);
        y = clip_rect->bottom;
        }
      else if(outcodeOut & BOTTOM)
        { // point is below the clip rectangle
        x = p1->x + (p2->x - p1->x) * (clip_rect->top - p1->y) / (p2->y - p1->y);
        y = clip_rect->top;
        }
      else if(outcodeOut & RIGHT)
        {  // point is to the right of clip rectangle
        y = p1->y + (p2->y - p1->y) * (clip_rect->right - p1->x) / (p2->x - p1->x);
        x = clip_rect->right;
        }
      else if(outcodeOut & LEFT)
        {   // point is to the left of clip rectangle
        y = p1->y + (p2->y - p1->y) * (clip_rect->left - p1->x) / (p2->x - p1->x);
        x = clip_rect->left;
        }

      // Now we move outside point to intersection point to clip
      // and get ready for next pass.
      if(outcodeOut == outcode0)
        {
        p1->x = x;
        p1->y = y;
        outcode0 = compute_out_code(p1, clip_rect);
        }
      else
        {
        p2->x = x;
        p2->y = y;
        outcode1 = compute_out_code(p2, clip_rect);
        }
      }
    }
  }

inline color_t alpha_blend(color_t pixel, color_t back, uint8_t weighting)
  {
  if(weighting == 0)
    return back;

  if(weighting == 255)
    return pixel;

  /* alpha blending the source and background colors */
  uint32_t rb = (((pixel & 0x00ff00ff) * weighting) + 
                ((back & 0x00ff00ff) * (0xff - weighting))) & 0xff00ff00;
  uint32_t g = (((pixel & 0x0000ff00) * weighting) + 
                ((back & 0x0000ff00) * (0xff - weighting))) & 0x00ff0000;
  
  return (pixel & 0xff000000) | ((rb | g) >> 8);
  }

typedef struct _points_t
  {
  point_t *buffer;
  size_t length;
  size_t end;
  } points_t;
  
typedef struct _flagged_points_t
  {
  points_t points;
  bool *flags;
  } flagged_points_t;
  
static result_t points_init(points_t *pts)
  {
  pts->buffer = (point_t *)malloc(sizeof(point_t) * 8);
  if(pts->buffer == 0)
    return e_not_enough_memory;
  
  pts->length = 8;
  pts->end = 0;
  return s_ok;
  }

static result_t points_free(points_t *pts)
  {
  if(pts == 0)
    return s_ok;
  
  free(pts->buffer);
  return s_ok;
  }

static size_t points_size(points_t *pts)
  {
  return pts->end;
  }

static result_t flagged_points_init(flagged_points_t *pts)
  {
  size_t alloc_size;
  if(points_init(&pts->points) != 0)
    return e_not_enough_memory;
  
  alloc_size =sizeof(bool) * pts->points.length;
  
  pts->flags = (bool *)malloc(alloc_size);
  if(pts->flags == 0)
    {
    points_free(&pts->points);
    return e_not_enough_memory;
    }
  
  memset(pts->flags, 0, alloc_size);
  
  return s_ok;
  }

static result_t flagged_points_free(flagged_points_t *pts)
  {
  if(pts == 0)
    return s_ok;
  
  points_free(&pts->points);
  free(pts->flags);
  return s_ok;
  }

static result_t expand_points(points_t *pts)
  {
  if(pts->end >= pts->length)
    {
    point_t *old = pts->buffer;
    size_t new_buffer = pts->length << 1;
    pts->buffer = (point_t *)malloc(new_buffer * sizeof(point_t));
    if(pts->buffer == 0)
      {
      pts->buffer = old;
      return e_not_enough_memory;
      }
    
    memcpy(pts->buffer, old, pts->length * sizeof(point_t));
    free(old);
    pts->length = new_buffer;
    }

  return s_ok;
  }

static result_t add_point(points_t *pts, const point_t *pt)
  {
  if(expand_points(pts)!= s_ok)
      return e_not_enough_memory;
  
  copy_point(pt, &pts->buffer[pts->end]);
  pts->end++;
  
  return s_ok;
  }

static result_t insert_point(points_t *pts, size_t at, const point_t *pt)
  {
  size_t cp;
  if(expand_points(pts)!= s_ok)
      return e_not_enough_memory;

  if(at == pts->end)
    return add_point(pts, pt);
  else if(at > pts->end)
    return e_bad_parameter;
  
  // can't use memcpy
  for(cp = pts->end; cp >= at; cp--)
    copy_point(&pts->buffer[cp-1], &pts->buffer[cp]);
  
  copy_point(pt, &pts->buffer[at]);
  pts->end++;
  
  return s_ok;
  }

static result_t assign_points(points_t *pts, const point_t *src, size_t len)
  {
  while(len > pts->length)
    if(expand_points(pts) != s_ok)
      return e_not_enough_memory;
  
  pts->end = len;
  memcpy(pts->buffer, src, sizeof(point_t) * len);
  
  return s_ok;
  }

static result_t expand_flagged_points(flagged_points_t *pts)
  {
  size_t old_size = pts->points.length;
  if(expand_flagged_points(pts)!= 0)
    return e_not_enough_memory;
  
  if(pts->points.length != old_size)
    {
    size_t new_buffer = pts->points.length;
    bool *old = pts->flags;
    pts->flags = (bool *)malloc(sizeof(bool) * new_buffer);
    if(pts->flags == 0)
      {
      pts->flags = old;
      pts->points.end--;
      return e_not_enough_memory;
      }
    memcpy(pts->flags, old, sizeof(bool) * old_size);
    free(old);
    }
  
  return s_ok;
  }

static result_t add_flagged_point(flagged_points_t *pts, const point_t *pt, bool flag)
  {
  if(expand_flagged_points(pts) != s_ok)
    return e_not_enough_memory;
  
  pts->flags[pts->points.end-1] = flag;
  return s_ok;
  }

static result_t insert_flagged_point(flagged_points_t *pts, size_t at, const point_t *pt, bool flag)
  {
  size_t cp;
  if(expand_flagged_points(pts)!= s_ok)
      return e_not_enough_memory;

  if(at == pts->points.end)
    return add_flagged_point(pts, pt, flag);
  else if(at > pts->points.end)
    return e_bad_parameter;
  
  // can't use memcpy
  for(cp = pts->points.end; cp >= at; cp--)
    {
    copy_point(&pts->points.buffer[cp-1], &pts->points.buffer[cp]);
    pts->flags[cp] = pts->flags[cp-1];
    }
  
  copy_point(pt, &pts->points.buffer[at]);
  pts->flags[at] = flag;
  pts->points.end++;
  
  return s_ok;
  }

inline size_t flagged_points_size(const flagged_points_t *pts)
  {
  return pts->points.end;
  }

static bool intersect_line(const point_t *p1, const point_t *p2, // line 1
    const point_t *p3, const point_t *p4,    // line 2
    point_t *ip)
  {
  // if lines do not intersect then exit
  gdi_dim_t d2 = (p1->x - p2->x) * (p3->y - p1->y) - (p1->y - p2->y) * (p3->x - p1->x);
  gdi_dim_t d3 = (p1->x - p2->x) * (p4->y - p1->y) - (p1->y - p2->y) * (p4->x - p1->x);

  if((d2 < 0 && d3 < 0) || (d2 > 0 && d3 > 0))
    return false;

  d2 = (p3->x - p4->x) * (p1->y - p3->y) - (p3->y - p4->y) * (p1->x - p3->x);
  d3 = (p3->x - p4->x) * (p2->y - p3->y) - (p3->y - p4->y) * (p2->x - p3->x);

  if((d2 < 0 && d3 < 0) || (d2 > 0 && d3 > 0))
    return false;

  gdi_dim_t d = (p1->x - p2->x) * (p3->y - p4->y)
                - (p1->y - p2->y) * (p3->x - p4->x);
  if(d == 0)
    return false;

  ip->x = ((p3->x - p4->x) * ((p1->x * p2->y) - (p1->y * p2->x))
      - (p1->x - p2->x) * ((p3->x * p4->y) - (p3->y * p4->x))) / d;
  ip->y = ((p3->y - p4->y) * ((p1->x * p2->y) - (p1->y * p2->x))
      - (p1->y - p2->y) * ((p3->x * p4->y) - (p3->y * p4->x))) / d;

  return true;
  }

inline gdi_dim_t distance(const point_t *p1, const point_t *p2)
  {
  return ((p2->x - p1->x) * (p2->x - p1->x)) + ((p2->y - p1->y) * (p2->y - p1->y));
  }

// is_left(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
//    See: Algorithm 1 "Area of Triangles and Polygons"
inline int is_left(const point_t *p0, const point_t *p1, const point_t *p2)
  {
  return ((p1->x - p0->x) * (p2->y - p0->y) - (p2->x - p0->x) * (p1->y - p0->y));
  }

// point_in_polygon(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
static bool point_in_polygon(const point_t *pt, const flagged_points_t *subject)
  {
  int wn = 0;    // the  winding number counter
  int i;

  // loop through all edges of the polygon
  for(i = 0; i < flagged_points_size(subject) - 1; i++)
    {   // edge from V[i] to  V[i+1]
    if(subject->points.buffer[i].y <= pt->y)
      {          // start y <= P.y
      if(subject->points.buffer[i+1].y > pt->y)      // an upward crossing
        if(is_left(&subject->points.buffer[i], &subject->points.buffer[i + 1], pt) > 0) // P left of  edge
          ++wn;            // have  a valid up intersect
      }
    else
      {                        // start y > P.y (no test needed)
      if(subject->points.buffer[i + 1].y <= pt->y)     // a downward crossing
        if(is_left(&subject->points.buffer[i], &subject->points.buffer[i + 1], pt) < 0) // P right of  edge
          --wn;            // have  a valid down intersect
      }
    }

  return wn > 0;
  }
typedef struct {
  point_t pt;
  size_t dist;
  } intersection_t;

static void polygon_intersect(const rect_t *clip_rect, const point_t *pts, size_t count, points_t *points)
  {
  // need to be freed
  flagged_points_t subject_points;
  points_t clip_points;
  
  point_t ip;
  size_t sp;
  size_t cp;
  int num_intersections = 0;
  int num_inside = 0;

  points_init(&clip_points);
  flagged_points_init(&subject_points);
  points->end = 0;

  // create a list of points and mark them as inside or outside the clipping rectangle
  for(sp = 0; sp < count; sp++)
    add_flagged_point(&subject_points, pts+sp, point_in_rect(pts + sp, clip_rect));
  // ensure is a closed polygon
  
  add_point(&clip_points, top_left(clip_rect, &ip));
  add_point(&clip_points, top_right(clip_rect, &ip));
  add_point(&clip_points, bottom_right(clip_rect, &ip));
  add_point(&clip_points, bottom_left(clip_rect, &ip));
  add_point(&clip_points, top_left(clip_rect, &ip));

  for(sp = 0; sp < flagged_points_size(&subject_points) - 1; sp++)
    {
    point_t line_start;
    
    copy_point(&subject_points.points.buffer[sp], &line_start);

    intersection_t intersections[2];
    size_t intersection = 0;

    if(point_in_rect(&line_start, clip_rect))
      num_inside++;

    // scan the clipping rectangle
    for(cp = 0; cp < points_size(&clip_points) - 1; cp++)
      {
      // an arc on the polygon can intersect at most 2 edges however we need to know the closest
      // to the arc start.
      if(intersect_line(&line_start, &subject_points.points.buffer[sp + 1],
          &clip_points.buffer[cp], &clip_points.buffer[cp + 1], &ip))
        {
        // when we have added an intersection we will find it again
        if(is_equal(&ip, &line_start))
          continue;

        copy_point(&ip, &intersections[intersection].pt);
        intersections[intersection].dist = cp;
        intersection++;
        }
      }

    if(intersection > 1)
      {
      // two intersections found for this line
      // make the first one the furthest distance
      if(distance(&line_start, &intersections[0].pt)
          < distance(&line_start, &intersections[1].pt))
        {
        size_t ts;
        
        swap_points(&intersections[0].pt, &intersections[1].pt);
        ts = intersections[0].dist;
        intersections[0].dist = intersections[1].dist;
        intersections[1].dist = ts;
        }
      }

    // intersections contains the ordered list of subject points
    while(intersection--)
      {
      copy_point(&intersections[intersection].pt, &ip);
      cp = intersections[intersection].dist;
      // check for case when points are on the clipping area
      if(!is_equal(&ip, &line_start) && 
         !is_equal(&ip, &subject_points.points.buffer[sp + 1]))
        {
        // we have a clipped line so add it to the result.
        insert_flagged_point(&subject_points, sp+1, &ip, true);
        insert_point(&clip_points, cp+1, &ip);
        if(intersection > 0)
          {
          sp++;     // only skip if there are 2 intersections
          if(intersections[0].dist > cp)
            intersections[0].dist++; // adjust for inserted intersection before edge start
          }
        num_intersections++;
        }
      }
    }

  if(num_intersections == 0)
    {
    if(num_inside > 0)
      assign_points(points, pts, count);
    else
      {
      // we need to see if the clip rect is inside the subject
      for(cp = 0; cp < points_size(&clip_points) - 1; cp++)
        {
        if(point_in_polygon(&clip_points.buffer[cp], &subject_points))
          {
          assign_points(points, clip_points.buffer, clip_points.end);
          return;
          }
        }
      }
    points_free(&clip_points);
    flagged_points_free(&subject_points);
    return;
    }

  // now we look for the first point on the subject that is inside, last point is first so ignore
  for(sp = 0; sp < subject_points.points.end - 1; sp++)
    if(subject_points.flags[sp])
      {
      add_point(points, &subject_points.points.buffer[sp]);
      sp++;
      break;
      }

  if(points->end == 0)
    {
    points_free(&clip_points);
    flagged_points_free(&subject_points);
    return;             // no result as all outside
    }

  copy_point(&points->buffer[0], &ip);       // point just inserted
  // walk the list
  do
    {
    if(subject_points.flags[sp])
      {
      // next point is clipped point
      copy_point(&subject_points.points.buffer[sp], &ip);
      }
    else
      {
      // find the point in the clip_points
      for(cp = 0; cp < clip_points.end - 1; cp++)
        if(is_equal(&clip_points.buffer[cp], &ip))
          break;                  // found last point inserted

      cp++;           // skip next on clipping rectangle
      if(cp == clip_points.end - 1)
        cp = 0;

      do
        {
        copy_point(&clip_points.buffer[cp], &ip);

        bool is_clipped = false;
        // see if the next point is on the subject
        for(sp = 0; sp < subject_points.points.end - 1; sp++)
          if(is_equal(&subject_points.points.buffer[sp], &ip))
            {
            is_clipped = true;
            break;
            }

        if(is_clipped)
          break;

        // point is part of the clipping rectangle.
        add_point(points, &ip);
        cp++;

        // wrap around to the start till we find an intersection point
        if(cp == clip_points.end - 1)
          cp = 0;
        }
      while(cp < clip_points.end - 1);
      }

    add_point(points, &ip);
    sp++;
    }
  while(!is_equal(&ip, &points->buffer[0]));
  
  points_free(&clip_points);
  flagged_points_free(&subject_points);
  }

result_t canvas_create_rect(const extent_t *size, handle_t *hndl)
  {
  return bsp_create_canvas_rect(size, hndl);
  }

result_t canvas_create_bitmap(const bitmap_t *bitmap,
                              handle_t *hndl)
  {
  extent_t size;
  rect_t rect;
  result_t result;
  gdi_dim_t x;
  gdi_dim_t y;
  gdi_dim_t row_offset = 0;
  
  size.dx = bitmap->bitmap_width;
  size.dy = bitmap->bitmap_height;
  if((result = bsp_create_canvas_rect(&size, hndl)) != s_ok)
    return result;
  
  rect.left = 0;
  rect.top = 0;
  rect.bottom = bitmap->bitmap_height;
  rect.right = bitmap->bitmap_width;
  
  // now set the pixels in the canvas
  for(y = 0; y < bitmap->bitmap_height; y++)
    {
    const color_t *line = bitmap->pixels + row_offset;
    for(x = 0; x < bitmap->bitmap_width; x++)
      {
      point_t pt = { x, y };
      canvas_set_pixel(*hndl, &rect, &pt, line[x]);
      }
    
    row_offset += bitmap->bitmap_width;
    }
  
  return s_ok;
  }

result_t canvas_close(handle_t hndl)
  {
  return bsp_canvas_close(hndl);
  }

result_t canvas_polyline(handle_t hndl,
                         const rect_t *clip_rect,
                         const pen_t *pen,
                         const point_t *points,
                         size_t count)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  gdi_dim_t half_width = pen->width >> 1;
  point_t p1;
  point_t p2;
  size_t pt;

  for(pt = 0; (pt + 1) < count; pt++)
    {
    copy_point(&points[pt], &p1);
    copy_point(&points[pt + 1], &p2);

    // clip the line to the clipping area
    if(!clip_line(&p1, &p2, clip_rect))
      continue;                 // line is outside the clipping area

    // ensure the line is always top->bottom
    if(p1.y > p2.y)
      swap_points(&p1, &p2);

    // draw the first pixel
    (*canvas->set_pixel)(canvas, &p1, pen->color);

    gdi_dim_t delta_x = p2.x - p1.x;
    gdi_dim_t delta_y = p2.y - p1.y;

    gdi_dim_t x_incr;
    if(delta_x >= 0)
      x_incr = 1;
    else
      {
      x_incr = -1;
      delta_x = -delta_x;
      }

    // we can optimize the drawing of horizontal and vertical lines
    if(delta_x == 0)
      {
      if(p1.y > p2.y)
        swap_points(&p1, &p2);

      while(p1.y < p2.y)
        {
        gdi_dim_t offset;
        (*canvas->set_pixel)(canvas, &p1, pen->color);

        for(offset = 1; offset <= half_width; offset++)
          {
          point_t p2;
          uint16_t *alpha_pt;
          copy_point(&p1, &p2);
          p2.x += offset;
          (*canvas->set_pixel)(canvas, &p2,
            alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));

          p2.x = p1.x;
          p2.x -= offset;
          (*canvas->set_pixel)(canvas, &p2,
            alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));
          }
        p1.y++;
        }
      }
    else if(delta_y == 0)
      {
      if(p1.x > p2.x)
        swap_points(&p1, &p2);

      while(p1.x < p2.x)
        {
        gdi_dim_t offset;
        (*canvas->set_pixel)(canvas, &p1, pen->color);

        for(offset = 1; offset <= half_width; offset++)
          {
          point_t p2;
          uint8_t *alpha_pt;
          
          copy_point(&p1, &p2);
          p2.y += offset;
          (*canvas->set_pixel)(canvas, &p2,
              alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));

          p2.y = p1.y;
          p2.y -= offset;
          (*canvas->set_pixel)(canvas, &p2,
              alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));
          }
        p1.x++;
        }
      }
    else if(delta_y == delta_x)
      {
      do
        {
        gdi_dim_t offset;
        (*canvas->set_pixel)(canvas, &p1, pen->color);
        p1.x += x_incr;
        p1.y++;

        for(offset = 1; offset <= half_width; offset++)
          {
          point_t p2;
          uint8_t *alpha_pt;
          p2.x = p1.x + offset;
          p2.y = p1.y + offset;
          (*canvas->set_pixel)(canvas, &p2,
              alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));

          p2.x = p1.x - offset;
          p2.y = p1.y - offset;
          (*canvas->set_pixel)(canvas, &p2,
              alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));
          }
        }
      while(--delta_y > 0);
      }
    else if(delta_y + delta_x != 0)   // more than 1 pixel, or a wide line
      {
      int16_t intensity_shift = 4;
      int16_t weighting_complement_mask = 0xFF;
      int16_t error_adj;                // intensity to weight color by
      int16_t error_acc = 0;
      int16_t weighting;
      int16_t error_acc_temp;
      point_t p_alias;
      //------------------------------------------------------------------------
      // determine independent variable (one that always increments by 1 (or -1) )
      // and initiate appropriate line drawing routine (based on first octant
      // always). the x and y's may be flipped if y is the independent variable.
      //------------------------------------------------------------------------
      if(delta_y > delta_x)
        {
        /* Y-major line; calculate 16-bit fixed-point fractional part of a
         pixel that X advances each time Y advances 1 pixel, truncating the
         result so that we won't overrun the endpoint along the X axis */
        error_adj =
            (uint16_t) ((((int32_t) delta_x) << 16) / (int32_t) delta_y);

        while(--delta_y) // process each point in the line one at a time (just use delta_y)
          {
          error_acc_temp = error_acc;  // remember the current accumulated error
          error_acc += error_adj;             // calculate error for next pixel

          if(error_acc <= error_acc_temp)
            p1.x += x_incr;

          p1.y++;										          // increment independent variable

          // if pen width > 1 then we use a modified algorithm
          if(pen->width == 1)
            {
            weighting = error_acc >> intensity_shift;

            (*canvas->set_pixel)(canvas, &p1,
                alpha_blend((*canvas->get_pixel)(canvas, &p1), pen->color, weighting));	// plot the pixel

            p_alias.x = p1.x + x_incr;
            p_alias.y = p1.y;

            (*canvas->set_pixel)(canvas, &p_alias,
                alpha_blend((*canvas->get_pixel)(canvas, &p_alias), pen->color,
                    (weighting ^ weighting_complement_mask)));
            }
          else
            {
            gdi_dim_t offset;
            (*canvas->set_pixel)(canvas, &p1, pen->color);

            for(offset = 1; offset <= half_width; offset++)
              {
              point_t p2;
              copy_point(&p1, &p2);
              p2.x += offset;
              (*canvas->set_pixel)(canvas, &p2,
                  alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));

              p2.x = p1.x;
              p2.x -= offset;
              (*canvas->set_pixel)(canvas, &p2,
                  alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));
              }
            }
          }
        }
      else
        {
        error_adj =
            (uint16_t) ((((int32_t) delta_y) << 16) / (int32_t) delta_x);

        while(--delta_x)// process each point in the line one at a time (just use delta_y)
          {
          error_acc_temp = error_acc;  // remember the current accumulated error
          error_acc += error_adj;             // calculate error for next pixel

          if(error_acc <= error_acc_temp)
            p1.y++;

          p1.x += x_incr;										  // increment independent variable

          if(pen->width == 1)
            {
            weighting = error_acc >> intensity_shift;

            (*canvas->set_pixel)(canvas, &p1,
                alpha_blend((*canvas->get_pixel)(canvas, &p1), pen->color, weighting));	// plot the pixel

            p_alias.x = p1.x;
            p_alias.y = p1.y + 1;

            (*canvas->set_pixel)(canvas, &p_alias,
                alpha_blend((*canvas->get_pixel)(canvas, &p_alias), pen->color,
                    (weighting ^ weighting_complement_mask)));
            }
          else
            {
            gdi_dim_t offset;
            (*canvas->set_pixel)(canvas, &p1, pen->color);

            for(offset = 1; offset <= half_width; offset++)
              {
              point_t p2;
              copy_point(&p1, &p2);
              p2.y += offset;
              (*canvas->set_pixel)(canvas, &p2,
                  alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));

              p2.y = p1.y;
              p2.y -= offset;
              (*canvas->set_pixel)(canvas, &p2,
                  alpha_blend((*canvas->get_pixel)(canvas, &p2), pen->color, 255 >> offset));
              }
            }
          }
        }
      }
    }
  }

result_t canvas_fill_rect(handle_t hndl,
                          const rect_t *clip_rect,
                          const rect_t *rect,
                          color_t color)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  rect_t bounds;
  extent_t size;
  
  assign_rect(rect, &bounds);
  intersect_rect(clip_rect, &bounds);
  rect_extents(&bounds, &size);

// if the extents are 0 then done
  if(size.dx == 0 || size.dy == 0)
    return s_ok;
  
  // as the bsp to fill the rectangle as quickly as possible
  (*canvas->fast_fill)(canvas, &bounds, color, rop_srccopy);

  return s_ok;
  }

result_t canvas_ellipse(handle_t hndl,
                        const rect_t *clip_rect,
                        const pen_t *pen,
                        color_t fill,
                        const rect_t *area)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  
  gdi_dim_t w = rect_width(area) >> 1;
  gdi_dim_t h = rect_height(area) >> 1;

  point_t c = { area->left + w, area->top + h };

// these are used to trace and draw the lines
  point_t q1;
  point_t q2;
  point_t q3;
  point_t q4;

// this holds the fill line
  point_t l1;
  point_t l2;

// we now use the width\height to calculate the ellipse
  gdi_dim_t a = w;
  gdi_dim_t b = h;
  gdi_dim_t x = 0;
  gdi_dim_t y = b;
  gdi_dim_t a_sqr = a * a;
  gdi_dim_t b_sqr = b * b;
  gdi_dim_t a22 = a_sqr + a_sqr;
  gdi_dim_t b22 = b_sqr + b_sqr;
  gdi_dim_t a42 = a22 + a22;
  gdi_dim_t b42 = b22 + b22;
  gdi_dim_t x_slope = b42;						// x_slope = (4 * b^^2) * (x * 1)
  gdi_dim_t y_slope = b42 * (y - 1);	// y_slope = (4 * a^^2) * (y - 1)
  gdi_dim_t mida = a_sqr >> 1;				// a^^2 / 2
  gdi_dim_t midb = b_sqr >> 1;				// b^^2 / 2
  gdi_dim_t d = a_sqr - (y_slope >> 1) - mida;	// subtract a^^2 / 2 to optimize

  bool draw_pen = pen->color != color_hollow;
  bool draw_fill = fill != color_hollow;
  
  q1.x = c.x;
  q1.y = area->top + (h << 1);
  q2.x = c.x;
  q2.y = area->top + (h << 1);
  q3.x = c.x;
  q3.y = area->top;
  q4.x = c.x;
  q4.y = area->top;

// region 1
  while(d <= y_slope)
    {
    // draw the ellipse point
    if(draw_pen && point_in_rect(&q1, clip_rect))
      (*canvas->set_pixel)(canvas, &q1, pen->color);

    if(!is_equal(&q1, &q2))
      {
      if(draw_pen && point_in_rect(&q2, clip_rect))
        (*canvas->set_pixel)(canvas, &q2, pen->color);

      if(draw_fill)
        {
        l1 = q1;
        l2 = q2;
        // fill the ellipse
        if(fill != (l2.x - l1.x) > 2)
          {
          l1.x++;
          if(clip_line(&l1, &l2, clip_rect))
            (*canvas->fast_line)(canvas, &l1, &l2, fill, rop_srccopy);
          }
        }
      }

    if(!is_equal(&q1, &q3) && draw_pen && point_in_rect(&q3, clip_rect))
      (*canvas->set_pixel)(canvas, &q3, pen->color);

    if(!is_equal(&q2, &q4) && !is_equal(&q3, &q4))
      {
      if(draw_pen && point_in_rect(&q4, clip_rect))
        (*canvas->set_pixel)(canvas, &q4, pen->color);

      l1 = q3;
      l2 = q4;
      // fill the ellipse
      if(draw_fill && (l2.x - l1.x) > 2)
        {
        l1.x++;
        if(clip_line(&l1, &l2, clip_rect))
          (*canvas->fast_line)(canvas, &l1, &l2, fill, rop_srccopy);
        }
      }

    if(d > 0)
      {
      d -= y_slope;
      y--;
      q1.y--;
      q2.y--;
      q3.y++;
      q4.y++;
      y_slope -= a42;
      }

    d += b22 + x_slope;
    x++;
    q1.x--;
    q2.x++;
    q3.x--;
    q4.x++;
    x_slope += b42;
    }

  d -= ((x_slope + y_slope) >> 1) + (b_sqr - a_sqr) + (mida - midb);
// optimized region change using x_slope, y_slope
// region 2
  while(y >= 0)
    {
    // draw the ellipse point
    if(draw_pen && point_in_rect(&q1, clip_rect))
      (*canvas->set_pixel)(canvas, &q1, pen->color);

    if(!is_equal(&q1, &q2))
      {
      if(draw_pen && point_in_rect(&q2, clip_rect))
        (*canvas->set_pixel)(canvas, &q2, pen->color);

      if(draw_fill)
        {
        l1 = q1;
        l2 = q2;
        // fill the ellipse
        if(fill != (l2.x - l1.x) > 2)
          {
          l1.x++;
          if(clip_line(&l1, &l2, clip_rect))
            (*canvas->fast_line)(canvas, &l1, &l2, fill, rop_srccopy);
          }
        }
      }

    if(!is_equal(&q1, &q3) && draw_pen && point_in_rect(&q3, clip_rect))
      (*canvas->set_pixel)(canvas, &q3, pen->color);

    if(!is_equal(&q2, &q4) && !is_equal(&q3, &q4))
      {
      if(draw_pen && point_in_rect(&q4, clip_rect))
        (*canvas->set_pixel)(canvas, &q4, pen->color);

      copy_point(&q3, &l1);
      copy_point(&q4, &l2);
      
      // fill the ellipse
      if(draw_fill && (l2.x - l1.x) > 2)
        {
        l1.x++;
        if(clip_line(&l1, &l2, clip_rect))
          (*canvas->fast_line)(canvas, &l1, &l2, fill, rop_srccopy);
        }
      }

    if(d <= 0)
      {
      d += x_slope;
      x++;
      q1.x--;
      q2.x++;
      q3.x--;
      q4.x++;
      x_slope += b42;
      }

    d += a22 - y_slope;
    y--;
    q1.y--;
    q2.y--;
    q3.y++;
    q4.y++;
    y_slope -= a42;
    }
  
  return s_ok;
  }

typedef struct _edge_t
  {
  gdi_dim_t x1, y1, x2, y2;
  gdi_dim_t cx, fn, mn, d;
  } edge_t;

inline bool edge_cmp(const edge_t *lp, const edge_t *rp)
  {
  /* if the minimum y values are different, sort on minimum y */
  if(lp->y1 != rp->y1)
    return lp->y1 < rp->y1;

  /* if the current x values are different, sort on current x */
  if(lp->cx != rp->cx)
    return lp->cx < rp->cx;

  /* otherwise they are equal */
  return false;
  }

static void swap_edge(edge_t *e1, edge_t *e2)
  {
  gdi_dim_t tmp;
  
  tmp = e1->x1;
  e1->x1 = e2->x1;
  e2->x1 = tmp;
  
  tmp = e1->y1;
  e1->y1 = e2->y1;
  e2->y1 = tmp;
  
  tmp = e1->x2;
  e1->x2 = e2->x2;
  e2->x2 = tmp;
  
  tmp = e1->y2;
  e1->y2 = e2->y2;
  e2->y2 = tmp;
  
  tmp = e1->cx;
  e1->cx = e2->cx;
  e2->cx = tmp;
  
  tmp = e1->fn;
  e1->fn = e2->fn;
  e2->fn = tmp;
  
  tmp = e1->mn;
  e1->mn = e2->mn;
  e2->mn = tmp;
  
  tmp = e1->d;
  e1->d = e2->d;
  e2->d = tmp;
  }

static void copy_edge(const edge_t *from, edge_t *to)
  {
  memcpy(to, from, sizeof(edge_t));
  }
 
static int partition(edge_t *a,int l,int u)
{
  edge_t v;
  int i;
  int j;
  edge_t temp;
    
  copy_edge(a+l, &v);

  i=l;
  j=u+1;
    
  do
  {
    do
      {
      i++;
      } while(edge_cmp(a+i, &v) && i<=u);
        
    do
      {
      j--;
      } while(edge_cmp(&v, a+j));
        
    if(i<j)
      swap_edge(a+i, a+j);

    }while(i<j);
    
  copy_edge(a + j, a+l);
  copy_edge(&v, a + j);
  return j;
  }

static void sort_edges(edge_t *a, int l, int u)
  {
  int j;
  if(l < u)
    {
    j = partition(a, l, u);
    sort_edges(a, l, j-1);
    sort_edges(a, j+1, u);
    }
  }

result_t canvas_polygon(handle_t hndl,
                        const rect_t *clip_rect,
                        const pen_t *outline,
                        color_t fill,
                        const point_t *pts,
                        size_t count,
                        bool interior_fill)
  {
  canvas_t *canvas = (canvas_t *)hndl;
// if the interior color is hollow then just a line draw
  if(fill == color_hollow)
    return canvas_polyline(hndl, clip_rect, outline, pts, count);
 
  pen_t fill_pen =
    {
    fill, 1, ps_solid
    };

// clip the polygon
  points_t points;
  if(points_init(&points) != s_ok)
    return e_not_enough_memory;
  
  polygon_intersect(clip_rect, pts, count, &points);

  // ignore small polygons
  if(points.end < 3)
    {
    points_free(&points);
    return e_bad_parameter;
    }

  edge_t *get; /* global edge table */
  int nge = 0; /* num global edges */
  int cge = 0; /* cur global edge */

  edge_t *aet; /* active edge table */
  int nae = 0; /* num active edges */

  int i, y;

  get = (edge_t *)malloc(points.end * sizeof(edge_t));
  
  if(get == 0)
    {
    points_free(&points);
    return e_not_enough_memory;
    }
  
  aet = (edge_t *)malloc(points.end * sizeof(edge_t));
  
  if(aet == 0)
    {
    free(get);
    points_free(&points);
    return e_not_enough_memory;
    }

  /* setup the global edge table */
  for(i = 0; i < count; ++i)
    {
    get[nge].x1 = points.buffer[i].x;
    get[nge].y1 = points.buffer[i].y;
    get[nge].x2 = points.buffer[(i + 1) % points.end].x;
    get[nge].y2 = points.buffer[(i + 1) % points.end].y;

    if(get[nge].y1 != get[nge].y2)
      {
      if(get[nge].y1 > get[nge].y2)
        {
        gdi_dim_t tmp;
        
        tmp = get[nge].x1;
        get[nge].x1 = get[nge].x2;
        get[nge].x2 = tmp;
        
        tmp = get[nge].y1;
        get[nge].y1 = get[nge].y2;
        get[nge].y2 = tmp;
        }
      
      get[nge].cx = get[nge].x1;
      get[nge].mn = get[nge].x2 - get[nge].x1;
      get[nge].d = get[nge].y2 - get[nge].y1;
      get[nge].fn = get[nge].mn / 2;
      ++nge;
      }
    }

  sort_edges(get, 0, nge-1);

  /* start with the lowest y in the table */
  y = get[0].y1;

  do
    {

    /* add edges to the active table from the global table */
    while((nge > 0) && (get[cge].y1 == y))
      {
      aet[nae] = get[cge++];
      --nge;
      aet[nae++].y1 = 0;
      }

    sort_edges(aet, 0, nae-1);

    /* using odd parity, render alternating line segments */
    for(i = 1; i < nae; i += 2)
      {
      int l = (int) aet[i - 1].cx;
      int r = (int) aet[i].cx;

      if(r > l)
        {
        point_t p1;
        point_t p2;
        
        p1.x = l;
        p1.y = y;
        
        p2.x = r;
        p2.y = y;
        (*canvas->fast_line)(canvas, &p1, &p2, fill, rop_srccopy); 
        // draw line between l and r and not between l and (r-1)
        }
      }

    /* prepare for the next scan line */
    ++y;

    /* remove inactive edges from the active edge table */
    /* or update the current x position of active edges */
    for(i = 0; i < nae; ++i)
      {
      if(aet[i].y2 == y)
        aet[i--] = aet[--nae];
      else
        {
        aet[i].fn += aet[i].mn;
        if(aet[i].fn < 0)
          {
          aet[i].cx += aet[i].fn / aet[i].d - 1;
          aet[i].fn %= aet[i].d;
          aet[i].fn += aet[i].d;
          }

        if(aet[i].fn >= aet[i].d)
          {
          aet[i].cx += aet[i].fn / aet[i].d;
          aet[i].fn %= aet[i].d;
          }
        }
      }
    /* keep doing this while there are any edges left */
    } while((nae > 0) || (nge > 0));

  /* all done, free the edge tables */
  free(aet);
  free(get);
  points_free(&points);

  // now outline the polygon using the pen
  if(outline != 0 && outline->color != color_hollow)
    return canvas_polyline(hndl, clip_rect, outline, pts, count);
  
  return s_ok;
  }

result_t canvas_rectangle(handle_t hndl,
                          const rect_t *clip_rect,
                          const pen_t *pen,
                          color_t color,
                          const rect_t *rect)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  result_t result;
  // draw the outline
  point_t pts[5] = {
    { rect->left, rect->top },
    { rect->left, rect->bottom },
    { rect->right, rect->bottom },
    { rect->right, rect->top },
    { rect->left, rect->top }
    };
  rect_t fill_rect = {
    rect->left + 1,
    rect->top -1,
    rect->right -1,
    rect->bottom -1
    };
  
  // the we fill the inside
  if((result = canvas_fill_rect(canvas, clip_rect, &fill_rect, color)) != s_ok)
    return result;

  return canvas_polyline(canvas, clip_rect, pen, pts, 5);
  }

result_t canvas_round_rect(handle_t hndl,
                           const rect_t *clip_rect,
                           const pen_t *pen,
                           color_t fill,
                           const rect_t *rect,
                           const extent_t *dim)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  result_t result;
  // a round rect is a series of lines and arcs.
  rect_t tmp;

  // all lines are drawn on the last pixel
  rect_t draw_rect;
  
  copy_rect(rect, &draw_rect);
  draw_rect.right--;
  draw_rect.bottom--;

  // fill
  if(fill != color_hollow)
    {

    // top bar
    tmp.top = draw_rect.top + 1;
    tmp.left = draw_rect.left + dim->dx;
    tmp.bottom = draw_rect.top + dim->dy + 1;
    tmp.right = draw_rect.right - dim->dx - 1;
    if((result = canvas_fill_rect(canvas, clip_rect, &tmp, fill))!= s_ok)
      return result;

    tmp.top = draw_rect.bottom - dim->dy;
    tmp.left = draw_rect.left + dim->dx + 1;
    tmp.right = draw_rect.right + dim->dy;
    tmp.bottom = draw_rect.bottom - dim->dy - 1;
    if((result = canvas_fill_rect(canvas, clip_rect, &tmp, fill))!= s_ok)
      return result;

    tmp.top = draw_rect.top + dim->dy + 1;
    tmp.left = draw_rect.left + 1;
    tmp.right = draw_rect.right - 1;
    tmp.bottom = draw_rect.bottom - dim->dy - 1;
    if((result = canvas_fill_rect(canvas, clip_rect, &tmp, fill)) != s_ok)
      return result;
    }
  // lines
  point_t pts[2];

  pts[0].x = draw_rect.left + dim->dx;
  pts[0].y = draw_rect.top;
    
  pts[1].x = draw_rect.right - dim->dx;
  pts[1].y = draw_rect.top;
  
  if((result = canvas_polyline(canvas, clip_rect, pen, pts, 2)) != s_ok)   // top
    return result;

  pts[0].x = draw_rect.right;
  pts[0].y = draw_rect.top + dim->dy;
  
  pts[1].x = draw_rect.right;
  pts[1].y = draw_rect.bottom - dim->dy;
  
  if((result = canvas_polyline(canvas, clip_rect, pen, pts, 2)) != s_ok)   // right
    return result;

  pts[0].x = draw_rect.left;
  pts[0].y = draw_rect.top + dim->dy;
  
  pts[1].x = draw_rect.left;
  pts[1].y = draw_rect.bottom - dim->dy;
  
  if((result = canvas_polyline(canvas, clip_rect, pen, pts, 2))!= s_ok)   // left
    return result;

  pts[0].x = draw_rect.left + dim->dx;
  pts[0].y = draw_rect.bottom;
  
  pts[1].x = draw_rect.right - dim->dx;
  pts[1].y = draw_rect.bottom;
  
  if((result = canvas_polyline(canvas, clip_rect, pen, pts, 2))!= s_ok)   // bottom
    return result;

  // Arcs

  // top left
  // clip the elipse
  tmp.left = draw_rect.left;
  tmp.top = draw_rect.top;
  tmp.right = draw_rect.left + dim->dx + 1;
  tmp.bottom = draw_rect.top + dim->dy + 1;

  intersect_rect(clip_rect, &tmp);

  rect_t ellrect;
  ellrect.left = draw_rect.left;
  ellrect.top = draw_rect.top;
  ellrect.right = draw_rect.left + (dim->dx << 1);
  ellrect.bottom = draw_rect.top + (dim->dy << 1);

  if((result = canvas_ellipse(canvas, &tmp, pen, fill, &ellrect)) != s_ok)
    return result;

  // bottom left
  tmp.left = draw_rect.left;
  tmp.top = draw_rect.bottom - dim->dy - 1;
  tmp.right = draw_rect.left + dim->dx + 1;
  tmp.bottom = draw_rect.bottom;

  intersect_rect(clip_rect,&tmp);

  ellrect.left = draw_rect.left;
  ellrect.top = draw_rect.bottom - (dim->dy << 1);
  ellrect.right = draw_rect.left + (dim->dx << 1);
  ellrect.bottom = draw_rect.bottom;

  if((result = canvas_ellipse(canvas, &tmp, pen, fill, &ellrect)) != s_ok)
    return result;

  // top right
  tmp.left = draw_rect.right - dim->dx - 1;
  tmp.top = draw_rect.top;
  tmp.right = draw_rect.right;
  tmp.bottom = draw_rect.top + dim->dy + 1;

  intersect_rect(clip_rect, &tmp);

  ellrect.left = draw_rect.right - (dim->dx << 1);
  ellrect.top = draw_rect.top;
  ellrect.right = draw_rect.right;
  ellrect.bottom = draw_rect.top + (dim->dy << 1);

  if((result = canvas_ellipse(canvas, &tmp, pen, fill, &ellrect)) != s_ok)
    return result;

  // bottom right
  tmp.left = draw_rect.right - dim->dx - 1;
  tmp.top = draw_rect.bottom - dim->dy - 1;
  tmp.right = draw_rect.right;
  tmp.bottom = draw_rect.bottom;

  intersect_rect(clip_rect, &tmp);

  ellrect.left = draw_rect.right - (dim->dx << 1);
  ellrect.top = draw_rect.bottom - (dim->dy << 1);
  ellrect.right = draw_rect.right;
  ellrect.bottom = draw_rect.bottom;

  return canvas_ellipse(canvas, &tmp, pen, fill, &ellrect);
  }

result_t canvas_bit_blt(handle_t hndl,
                        const rect_t *clip_rect,
                        const rect_t *dest_rect,
                        handle_t src_canvas,
                        const rect_t *src_clip_rect,
                        const point_t *src_pt,
                        raster_operation rop)
  {
  
  canvas_t *canvas = (canvas_t *)hndl;
  // if the src point is outside the source then done
  if(!point_in_rect(src_pt, src_clip_rect))
    return s_ok;

// if the destination is to the left of teh clip-rect then
// adjust both it and the src-rect

// determine how much to shift the left by
  gdi_dim_t dst_left = dest_rect->left > clip_rect->left ? dest_rect->left : clip_rect->left;
  gdi_dim_t src_left_offset =
      dst_left > dest_rect->left ? dst_left - dest_rect->left : 0;

  gdi_dim_t dst_top = dest_rect->top > clip_rect->top ? dest_rect->top : clip_rect->top;
  gdi_dim_t src_top_offset =
      dst_top > dest_rect->top ? dst_top - dest_rect->top : 0;

  gdi_dim_t dst_right = dest_rect->right < clip_rect->right ? dest_rect->right : clip_rect->right;
  gdi_dim_t src_right_offset =
      dest_rect->right < dst_right ? dst_right - dest_rect->right : 0;

  gdi_dim_t dst_bottom = dest_rect->bottom < clip_rect->bottom ? dest_rect->bottom : clip_rect->bottom;
  gdi_dim_t src_bottom_offset =
      dest_rect->bottom > dst_bottom ? dst_bottom - dest_rect->bottom : 0;

  rect_t destination =
    { dst_left, dst_top, dst_right, dst_bottom };

  extent_t dst_size =
    { rect_width(&destination), rect_height(&destination) };

  if(dst_size.dx == 0 || dst_size.dy == 0)
    return s_ok;

  rect_t source = {
    src_pt->x + src_left_offset,
    src_pt->y + src_top_offset,
    src_pt->x + rect_width(dest_rect) +  src_right_offset,
    src_pt->y + rect_height(dest_rect) + src_bottom_offset
    };

  intersect_rect(src_clip_rect, &source);

  extent_t src_size = {
    rect_width(&source),
    rect_height(&source)
    };

  if(src_size.dx == 0 || src_size.dy == 0)
    return s_ok;
  
  point_t dst_pt = { destination.left, destination.top };

  (*canvas->fast_copy)(canvas, &dst_pt, (canvas_t *)src_canvas, &source, rop);
  
  return s_ok;
  }

result_t canvas_mask_blt(handle_t hndl,
                         const rect_t *clip_rect,
                         const rect_t *dest_rect,
                         handle_t src_canvas,
                         const rect_t *src_clip_rect,
                         const point_t *src_point,
                         const bitmap_t *mask_bitmap,
                         const point_t *mask_point,
                         raster_operation operation)
  {
  return e_not_implemented;
  }

result_t canvas_rotate_blt(handle_t hndl,
                           const rect_t *clip_rect,
                           const point_t *dest_center,
                           handle_t src,
                           const rect_t *src_clip_rect,
                           const point_t *src_point,
                           gdi_dim_t radius,
                           int angle,
                           raster_operation operation)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  canvas_t *src_canvas = (canvas_t *)src;
  // we perform this operation only on the circle bounded however for
  // speed we perform this as a rectangle
  gdi_dim_t r2 = radius * radius;

  // assign the start and end to the rectangle bounds relative to the origin
  gdi_dim_t init_x;
  gdi_dim_t y;
  gdi_dim_t right;
  gdi_dim_t bottom;
  
  if(angle < 0 || angle > 359)
    return e_bad_parameter;

  if(src_point->x - src_clip_rect->left < radius)
    init_x = src_clip_rect->left;
  else
    init_x = src_point->x - radius;

  if(src_clip_rect->right - src_point->y < radius)
    right = src_clip_rect->right;
  else
    right = src_point->x + radius;

  if(src_point->y - src_clip_rect->top < radius)
    y = src_clip_rect->top;
  else
    y = src_point->y - radius;

  if(src_clip_rect->bottom - src_point->y < radius)
    bottom = src_clip_rect->bottom;
  else
    bottom = src_point->y + radius;

  for(; y < bottom; y++)
    {
    gdi_dim_t x;
    for(x = init_x; x < right; x++)
      {
      int yp = y - src_point->y;
      int xp = x - src_point->x;
      // simple pythagorus theorum to check the inclusion
      if(((yp * yp) + (xp * xp)) > r2)
        continue;
      
      point_t pt = { x, y };

      color_t cr =
          (*src_canvas->get_pixel)(src_canvas, &pt);

      if(cr != color_hollow && (operation != rop_srcpaint || cr != 0))
        {
        rotate_point(src_point, &pt, angle);

        yp = pt.y - src_point->y;
        xp = pt.x - src_point->x;
        
        point_t dest_pt = { dest_center->x + xp, dest_center->y + yp };

        (*canvas->set_pixel)(canvas, &dest_pt, cr);
        }
      }
    }
  
  return s_ok;
  }

color_t canvas_get_pixel(handle_t hndl,
                         const rect_t *clip_rect,
                         const point_t *pt)
  {
  canvas_t *canvas;
  if(!point_in_rect(pt, clip_rect))
    return RGB(0,0,0);
  
  canvas = (canvas_t *)hndl;
  return (*canvas->get_pixel)(canvas, pt);
  }

color_t canvas_set_pixel(handle_t hndl,
                         const rect_t *clip_rect,
                         const point_t *pt,
                         color_t c)
  {
  canvas_t *canvas;
  color_t old_pixel;
  if(!point_in_rect(pt, clip_rect))
    return RGB(0,0,0);
  
  canvas = (canvas_t *)hndl;
  old_pixel = (*canvas->get_pixel)(canvas, pt);
  (*canvas->set_pixel)(canvas, pt, c);
  return old_pixel;
  }

static const int arctans[45] = {
  0, 17450, 34878, 52264, 69587,
  86824, 103956, 120961, 137819, 154508,
  171010, 187303, 203368, 219186, 234736,
  250000, 264960, 279596, 293893, 307831,
  321394, 334565, 347329, 359670, 371572,
  383022, 394005, 404508, 414519, 424024,
  433013, 441474, 449397, 456773, 463592,
  469846, 475528, 480631, 485148, 489074,
  492404, 495134, 497261, 498782, 499695
  };

static int lookup_atan2(float scale, int y, int x)
  {
  int prod = (int)((y * scale) * (x * scale));
  int s;
  // find the offset
  if(prod < arctans[5])
    s = 0;
  else if(prod < arctans[10])
    s = 5;
  else if(prod < arctans[15])
    s = 10;
  else if(prod < arctans[20])
    s = 15;
  else if(prod < arctans[25])
    s = 20;
  else if(prod < arctans[30])
    s = 25;
  else if(prod < arctans[35])
    s = 30;
  else if(prod < arctans[40])
    s = 35;
  else
    s = 40;
  
  if(arctans[s +1] > prod)
    return s;
  
  if(arctans[s +2] > prod)
    return s+1;
  
  if(arctans[s +3] > prod)
    return s+2;
  
  if(arctans[s +4] > prod)
    return s+3;
  
  return s +4;
  }

result_t canvas_arc(handle_t hndl,
                    const rect_t *clip_rect,
                    const pen_t *pen,
                    const point_t *p,
                    gdi_dim_t radius,
                    int start_angle,
                    int end_angle)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  // normalise the drawing numbers
  if(start_angle > end_angle)
    {
    int tmp = start_angle;
    start_angle = end_angle;
    end_angle = tmp;
    }

  point_t pt = { radius, 0 };

  gdi_dim_t decision_over2 = 1 - pt.x;

  gdi_dim_t offset = pen->width >> 1;

  extent_t rect_offset = { 0 - offset, 0 - offset };
  extent_t wide_pen = { pen->width, pen->width };
  
  // our angles are 0..44 but our radius varies.  We need to calculate from
  // a given x and y coordinate the approximate angle.
  float scale = 1000.0f / radius;
  
  point_t dp;
  rect_t dr;

  while(pt.y <= pt.x)
    {
    int angle = lookup_atan2(scale, pt.y, pt.x);
    
    // double angle = atan2((double) pt.y, (double) pt.x);

    if(pen->width == 1)
      {
      // forward angles first
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(pt.x + p->x, pt.y + p->y, &dp), pen->color); // octant 1

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(-pt.y + p->x, pt.x + p->y, &dp), pen->color); // octant 3

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(-pt.x + p->x, -pt.y + p->y, &dp), pen->color); // octant 5

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(pt.y + p->x, -pt.x + p->y, &dp), pen->color); // octant 7

      angle -= 270;
      angle *= -1;
      angle += 90;

      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(pt.y + p->x, pt.x + p->y, &dp), pen->color); // octant 2

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(-pt.x + p->x, pt.y + p->y, &dp), pen->color); // octant 4

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(-pt.y + p->x, -pt.x + p->y, &dp), pen->color); // octant 6

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_set_pixel(canvas, clip_rect, make_point(pt.x + p->x, -pt.y + p->y, &dp), pen->color); // octant 8
      }
    else
      {
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(pt.x + p->x, pt.y + p->y, &dp), &wide_pen, &dr))); // octant 1

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
           offset_rect(&rect_offset,  make_rect(make_point(-pt.y + p->x, pt.x + p->y, &dp), &wide_pen, &dr))); // octant 3

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(-pt.x + p->x, -pt.y + p->y, &dp), &wide_pen, &dr))); // octant 5

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(pt.y + p->x, -pt.x + p->y, &dp), &wide_pen, &dr))); // octant 7

      angle -= 270;
      angle *= -1;
      angle += 90;

      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(pt.y + p->x, pt.x + p->y, &dp), &wide_pen, &dr))); // octant 2

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(-pt.x + p->x, pt.y + p->y, &dp), &wide_pen, &dr))); // octant 4

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(-pt.y + p->x, -pt.x + p->y, &dp), &wide_pen, &dr))); // octant 6

      angle += 90;
      if(angle >= start_angle && angle <= end_angle)
        canvas_ellipse(canvas, clip_rect, pen, pen->color,
            offset_rect(&rect_offset, make_rect(make_point(pt.x + p->x, -pt.y + p->y, &dp), &wide_pen, &dr))); // octant 8
      }

    pt.y++;
    if(decision_over2 <= 0)
      decision_over2 += 2 * pt.y + 1; // Change in decision criterion for y -> y+1
    else
      {
      pt.x--;
      decision_over2 += 2 * (pt.y - pt.x) + 1;  // Change for y -> y+1, x -> x-1
      }
    }
  
  return s_ok;
  }

result_t canvas_pie(handle_t hndl,
                    const rect_t *clip_rect,
                    const pen_t *pen,
                    color_t c,
                    const point_t *p,
                    int start,
                    int end,
                    gdi_dim_t radii,
                    gdi_dim_t inner)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  
  if(end < start)
    {
    int t = start;
    start = end;
    end = t;
    }

// special case for start == end
  if(start == end)
    {
    point_t pts[2] = {
        { p->x + inner, p->y },
        { p->x + radii, p->y }
      };

    rotate_point(p, &pts[0], start);
    rotate_point(p, &pts[1], start);

    return canvas_polyline(canvas, clip_rect, pen, pts, 2);
    }
// we draw a polygon using the current pen
// and brush.  We first need to calculate the outer
// point count
  
  float outer_incr = atan2(1.0, radii);
  float delta = end - start;

  delta = (delta < 0) ? -delta : delta;

  gdi_dim_t segs = (gdi_dim_t)(delta / outer_incr) + 1;

  float inner_incr;

// see if we have an inner radii
  if(inner > 0)
    {
    inner_incr = atan2(1.0, inner);
    segs += (gdi_dim_t)(delta / inner_incr) + 1;
    }
  else
    segs++;

// We allocate 1 more point so that the polyline will fill the area.
  point_t *pts = malloc(sizeof(point_t) * (segs + 1));
   
  int pt = 1;
  copy_point(p, pts);
  double theta;
  for(theta = start; theta <= end; theta += outer_incr)
    {
    pts[pt].x = p->x + radii;
    pts[pt].y = p->y;
    rotate_point(p, pts + pt, theta);
    pt++;
    }

  if(inner > 0)
    {
    for(theta = end; theta >= start; theta -= inner_incr)
      {
      pts[pt].x = p->x + inner;
      pts[pt].y = p->y;
      rotate_point(p, pts + pt, theta);
      pt++;
      }
    }
  else
    copy_point(p, pts +pt);

  canvas_polygon(canvas, clip_rect, pen, c, pts, segs + 1, false);
  
  free(pts);
  
  return s_ok;
  }

result_t canvas_draw_text(handle_t hndl,
                          const rect_t *clip_rect,
                          const font_t *font,
                          color_t fg,
                          color_t bg,
                          const char *str,
                          size_t count,
                          const point_t *pt,
                          const rect_t *txt_clip_rect,
                          text_flags format,
                          size_t *char_widths)
  {
  canvas_t *canvas = (canvas_t *)hndl;
  // create a clipping rectangle as needed
  rect_t txt_rect;
  point_t pt_pos = { pt->x, pt->y };
  
  copy_rect(clip_rect, &txt_rect);

  if(format & eto_clipped)
    intersect_rect(txt_clip_rect, &txt_rect);

  size_t ch;
  // a font is a monochrome bitmap.
  for(ch = 0; ch < count; ch++)
    {
    // get the character to use.  If the character is outside the
    // map then we use the default char
    char c = str[ch];

    if(c > font->last_char || c < font->first_char)
      c = font->default_char;

    c -= font->first_char;

    size_t cell_width = font->char_table[c << 1];
    // see if the user wants the widths returned
    if(char_widths != 0)
      char_widths[ch] = cell_width;

    size_t offset = font->char_table[(c << 1) + 1];
    const uint8_t *mask = font->bitmap_pointer + offset;

    size_t columns = (((cell_width - 1) | 7) + 1) >> 3;
    size_t col;
    for(col = 0; col < columns; col++)
      {
      size_t row;
      for(row = 0; row < font->bitmap_height; row++)
        {
        size_t bit = 0;
        size_t pel;
        
        for(pel = (col << 3); bit < 8 && pel < cell_width; pel++, bit++)
          {
          point_t pos = {
            (gdi_dim_t)(pt_pos.x + pel),
            (gdi_dim_t)(pt_pos.y + row)
            };

          bool is_fg = (*mask & (0x80 >> bit)) != 0;

          if(point_in_rect(&pos, clip_rect ))
            {
            if(is_fg)
              (*canvas->set_pixel)(canvas, &pos, fg);
            else if(format & eto_opaque)
              (*canvas->set_pixel)(canvas, &pos, bg);
            }
          }

        // increment past the current row
        mask++;
        }
      }

    pt_pos.x += (gdi_dim_t)(cell_width);
    }
  }

extent_t canvas_text_extent(handle_t hndl,
                            const font_t *font,
                            const char *str,
                            size_t count)
  {
  extent_t ex = { 0, font->bitmap_height };
  size_t ch;
  
  for(ch = 0; ch < count; ch++)
    {
    // get the character to use.  If the character is outside the
    // map then we use the default char
    char c = str[ch];
    if(c == 0)
      break;							// end of the string

    if(c > font->last_char || c < font->first_char)
      c = font->default_char;

    c -= font->first_char;

    ex.dx += font->char_table[c << 1];
    }

  return ex;
  }

result_t canvas_invalidate_rect(handle_t hndl,
                                const rect_t *rect)
  {
  return s_ok;
  }

