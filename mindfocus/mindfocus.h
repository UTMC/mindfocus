#ifndef __mindfocus_h__
#define __mindfocus_h__

struct _position{
  int win_x;
  int win_y;
  int win_w;

  int from_left;
  int from_right;
  int percent;
  int param;
  int method;
};
typedef struct _position POSITION;

void set_pos_by_per(int pos);
int get_pos_by_per();
void chg_grp(int num);

#endif /* __mindfocus_h__ */
