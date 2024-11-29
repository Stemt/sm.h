#include <stdbool.h>
#include <stdio.h>

#define SM_IMPLEMENTATION
#include "sm.h"

#define SIZE 5

typedef enum{
  Direction_NORTH = 0,
  Direction_NORTH_EAST,
  Direction_EAST,
  Direction_SOUTH_EAST,
  Direction_SOUTH,
  Direction_SOUTH_WEST,
  Direction_WEST,
  Direction_NORTH_WEST,
  Direction_END
} Direction;

void Direction_to_coord_offset(Direction direction, int* x, int* y){
  if(x == NULL) return;
  if(y == NULL) return;
  switch (direction) {
    case Direction_NORTH: 
      *x=0;
      *y=-1;
      return;
    case Direction_NORTH_EAST: 
      *x=1;
      *y=-1;
      return;
    case Direction_EAST: 
      *x=1;
      *y=0;
      return;
    case Direction_SOUTH_EAST: 
      *x=1;
      *y=1;
      return;
    case Direction_SOUTH: 
      *x=0;
      *y=1;
      return;
    case Direction_SOUTH_WEST: 
      *x=-1;
      *y=-1;
      return;
    case Direction_WEST: 
      *x=-1;
      *y=0;
      return;
    case Direction_NORTH_WEST: 
      *x=-1;
      *y=1;
      return;
    case Direction_END: break;
  }
  assert(false && "unreachable");
}

SM_def(sm);

typedef struct{
  int x;
  int y;
  bool alive;
  bool next_alive;
  void* grid;
  SM_Context sm_context;
} Cell;

typedef struct{
  Cell cells[SIZE][SIZE];
} Grid;

void Grid_init(Grid* self){
  for(int y = 0; y < SIZE; ++y){
    for(int x = 0; x < SIZE; ++x){
      self->cells[y][x] = (Cell){
        .next_alive = false,
        .alive = false,
        .grid = self,
        .x = x,
        .y = y,
        .sm_context = {0}
      };
      SM_Context_init(
          &self->cells[y][x].sm_context, 
          &self->cells[y][x]);
    }
  }
}

void Grid_print(Grid* self){
  for(int y = 0; y < SIZE; ++y){
    for(int x = 0; x < SIZE; ++x){
      printf("%c", self->cells[y][x].alive ? '#' : ' ');
    }
    printf("\n");
  }
}

Cell* Grid_get_cell(Grid* self, int x, int y){
  while(x < 0) x+=SIZE;
  if(x >= SIZE) x%=SIZE;
  while(y < 0) y+=SIZE;
  if(y >= SIZE) y%=SIZE;
  return &self->cells[y][x];
}

void Grid_update(Grid* self){
  // determine new states for cells
  for(int y = 0; y < SIZE; ++y){
    for(int x = 0; x < SIZE; ++x){
      SM_step(sm, &Grid_get_cell(self, x, y)->sm_context);
    }
  }

  // sync cell states to new state
  for(int y = 0; y < SIZE; ++y){
    for(int x = 0; x < SIZE; ++x){
      Cell* cell = Grid_get_cell(self, x, y);
      cell->alive = cell->next_alive;
    }
  }
}

Cell* Cell_get_neighbor(Cell* self, Direction direction){
  int x = 0, y = 0;
  Direction_to_coord_offset(direction, &x, &y);
  Cell* neighbor = Grid_get_cell(self->grid, self->x+x, self->y+y);
  return neighbor;
}

void Cell_state_alive_enter(void* ctx){
  Cell* self = ctx;
  self->next_alive = true;
}

void Cell_state_dead_enter(void* ctx){
  Cell* self = ctx;
  self->next_alive = false;
}

int Cell_count_alive_neighors(Cell* self){
  int alive_neighbors = 0;
  for(Direction direction = 0; direction < Direction_END; ++direction){
    Cell* neighbor = Cell_get_neighbor(self, direction);
    if(neighbor->alive) alive_neighbors++;
  }
  return alive_neighbors;
}

bool Cell_alive_to_dead_guard(void* ctx){
  Cell* self = ctx;
  int alive_neighbors = Cell_count_alive_neighors(self);
  return alive_neighbors < 2 || alive_neighbors > 3;
}

bool Cell_dead_to_alive_guard(void* ctx){
  Cell* self = ctx;
  int alive_neighbors = Cell_count_alive_neighors(self);
  return alive_neighbors == 3;
}

bool Cell_initial_to_alive_guard(void* ctx){
  Cell* self = ctx;
  return self->alive;
}

int main(void){

  // cell states 
  SM_State_create(alive);
  SM_State_set_enter_action(alive, Cell_state_alive_enter);

  SM_State_create(dead);
  SM_State_set_enter_action(dead, Cell_state_dead_enter);

  // initial transition with guard if cell is alive
  SM_Transition_create(sm, initial_to_alive, SM_INITIAL_STATE, alive);
  SM_Transition_set_guard(initial_to_alive, Cell_initial_to_alive_guard); 

  // initial transition if not alive 
  // (no guard or trigger -> trigger if other transitions failed to trigger)
  SM_Transition_create(sm, initial_to_dead, SM_INITIAL_STATE, dead);
  
  // transitions implementing the rules for GoL 
  // (https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)
  SM_Transition_create(sm, alive_to_dead, alive, dead);
  SM_Transition_set_guard(alive_to_dead, Cell_alive_to_dead_guard);
  
  SM_Transition_create(sm, dead_to_alive, dead, alive);
  SM_Transition_set_guard(dead_to_alive, Cell_dead_to_alive_guard);

  Grid grid = {0};
  Grid_init(&grid);
  
  // flyer
  grid.cells[0][0].alive = true;
  grid.cells[1][1].alive = true;
  grid.cells[2][1].alive = true;
  grid.cells[0][2].alive = true;
  grid.cells[1][2].alive = true;

  // performs initial transitions
  Grid_update(&grid);

  int steps = 5;
  for(int step = 0; step < steps; ++step){
    printf("--- GoL step: %02d ---\n",step);
    Grid_print(&grid);
    Grid_update(&grid);

  }
}
