#include "Fighter.h"

#include "InputManager.h"
#include "Camera.h"

#define LAYER 0

#define IDLE Fighter::FighterState::IDLE
#define LEFT Fighter::FighterState::LEFT
#define RIGHT Fighter::FighterState::RIGHT
#define JUMPING Fighter::FighterState::JUMPING
#define FALLING Fighter::FighterState::FALLING
#define SLIDING Fighter::FighterState::SLIDING


//TODO reavaliar se precisa ou não de Camera
Fighter::Fighter(string name, float x, float y){
  sprite[IDLE] = Sprite(name + "/idle.png", 6, 1);
  sprite[LEFT] = Sprite(name + "/left.png", 6, 1);
  sprite[RIGHT] = Sprite(name + "/right.png", 8, 1);
  sprite[JUMPING] = Sprite(name + "/jumping.png", 6, 1);
  sprite[FALLING] = Sprite(name + "/falling.png", 6, 1);
  sprite[SLIDING] = Sprite(name + "/sliding.png", 6, 1);

  state = IDLE;

  linear_speed = rotation = 0;
  speed = Vector(0, 0);
  acceleration = Vector(0, 0.2);

  box = Rectangle(x, y, sprite[state].get_width(), sprite[state].get_height());
}

Fighter::~Fighter(){
}

void Fighter::update(float delta){
  InputManager inputManager = InputManager::get_instance();

  speed.x = 0;
  if(inputManager.is_key_down(SDLK_a)){
    change_state(LEFT);
    speed.x = -1;
  }

  if(inputManager.is_key_down(SDLK_d)){
    change_state(RIGHT);
    speed.x = 1;
  }

  if(inputManager.is_key_down(SDLK_s)){
    change_state(SLIDING);
  }

  if(inputManager.is_key_down(SDLK_SPACE) && state != JUMPING){
    speed.y = -10;
  }

  if(speed.x == 0 && speed.y == 0 && not inputManager.is_key_down(SDLK_s)){
    change_state(IDLE);
  }

  if(speed.y > 0){
    change_state(FALLING);
  }else if(speed.y > 0){
    change_state(JUMPING);
  }

  speed.y += acceleration.y * delta;

  box.x += speed.x * delta;
  box.y += speed.y * delta;

//FIXME com colisão
  if(box.y + box.height * 0.5 > 500 && speed.y > 0){
    speed.y = 0;
    box.y = 500 - box.height * 0.5;
  }

  sprite[state].update(delta);
}

void Fighter::render(){
  int x = box.get_draw_x()  + 0 * Camera::pos[LAYER].x;
  int y = box.get_draw_y() + 0 * Camera::pos[LAYER].x;
  sprite[state].render(x, y, rotation);
}

bool Fighter::is_dead(){
 return false;
}

void Fighter::notify_collision(GameObject &){
}

bool Fighter::is(string type){
  return type == "fighter";
}

void Fighter::change_state(FighterState cstate){
  if(state == cstate) return;

  float old_width = sprite[state].get_width();
  float old_height = sprite[state].get_height();
  state = cstate;
  float new_width = sprite[state].get_width();
  float new_height = sprite[state].get_height();

  float x = box.x - (new_width - old_width) * 0.5;
  float y = box.y - (new_height - old_height) * 0.5;

  box = Rectangle(x, y, sprite[state].get_width(), sprite[state].get_height());
}