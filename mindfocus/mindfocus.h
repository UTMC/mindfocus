#ifndef __mindfocus_h__
#define __mindfocus_h__

struct _position{
  int win_x;
  int win_y;
  int win_w;
  int win_h;

  int from_left;
  int from_right;
  int x_percent;
  int x_param;
  int x_method;

  int from_top;
  int from_bottom;
  int y_percent;
  int y_param;
  int y_method;
};
typedef struct _position POSITION;

void set_x_pos_by_per(int x_pos);
int get_x_pos_by_per();
void set_x_pos_from_left(int x_pos);
int get_x_pos_from_left();
void set_x_pos_from_right(int x_pos);
int get_x_pos_from_right();
void set_y_pos_by_per(int y_pos);
int get_y_pos_by_per();
void set_y_pos_from_top(int y_pos);
int get_y_pos_from_top();
void set_y_pos_from_bottom(int y_pos);
int get_y_pos_from_bottom();
void chg_grp(int num);

#endif /* __mindfocus_h__ */
