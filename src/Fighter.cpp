#include "Fighter.h"

#include "InputManager.h"
#include "Floor.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#define LAYER 0

#define ii pair<int, int>

#define PI 3.14159265358979

#define CROUCH_COOLDOWN 100.0

using std::pair;
using std::vector;

Fighter::Fighter(int cjoystick_id){
	state = FighterState::IDLE;
	joystick_id = cjoystick_id;
	remaining_life = MAX_LIFE / 2;
	special = 0;
	attack_damage = 0;
	vertical_speed = rotation = 0;
	speed = Vector(0, 0);
	acceleration = Vector(0, 0.1);
	max_speed = 9;
	attack_mask = 0;
	sprite = vector<Sprite>(LAST);

	orientation = RIGHT;

	on_floor = false;
	last_collided_floor = 0;
	grab = false;
	pass_through = false;

	n_sprite_start = 0;
	tags["player"] = true;
}

Fighter::~Fighter(){
}

void Fighter::process_input(){
	InputManager * input_manager = InputManager::get_instance();

	vector< pair<int, int> > buttons = {
		ii(JUMP_BUTTON, SDLK_SPACE),
		ii(UP_BUTTON, SDLK_w),
		ii(DOWN_BUTTON, SDLK_s),
		ii(LEFT_BUTTON, SDLK_a),
		ii(RIGHT_BUTTON, SDLK_d),
		ii(ATTACK_BUTTON, SDLK_l),
		ii(SPECIAL1_BUTTON, SDLK_o),
		ii(SPECIAL2_BUTTON, SDLK_k),
		ii(BLOCK_BUTTON, SDLK_i)
	};

	vector< pair<int, int> > joystick_buttons = {
		ii(JUMP_BUTTON, InputManager::A),
		ii(UP_BUTTON, InputManager::UP),
		ii(DOWN_BUTTON, InputManager::DOWN),
		ii(LEFT_BUTTON, InputManager::LEFT),
		ii(RIGHT_BUTTON, InputManager::RIGHT),
		ii(ATTACK_BUTTON, InputManager::X),
		ii(SPECIAL1_BUTTON, InputManager::LT),
		ii(SPECIAL2_BUTTON, InputManager::RT),
		ii(BLOCK_BUTTON, InputManager::RB)
	};

	if(joystick_id != -1){
		for(ii button : joystick_buttons){
			pressed[button.first] = input_manager->joystick_button_press(button.second, joystick_id);
			is_holding[button.first] = input_manager->is_joystick_button_down(button.second, joystick_id);
			released[button.first] = input_manager->joystick_button_release(button.second, joystick_id);
		}
	}else{
		for(ii button : buttons){
			pressed[button.first] = input_manager->key_press(button.second, true);
			is_holding[button.first] = input_manager->is_key_down(button.second, true);
			released[button.first] = input_manager->key_release(button.second, true);
		}
	}
}

void Fighter::update(float delta){
	process_input();

	temporary_state = state;

	sprite[state].update(delta);

	update_machine_state();

	speed.y = std::min(speed.y + !on_floor * acceleration.y * delta, max_speed);
	box.x += speed.x * delta;
	if(not on_floor) box.y += speed.y * delta;

	test_limits();

	crouch_timer.update(delta);

	change_state(temporary_state);

	speed.x = 0;
	grab = false;
	on_floor = false;
}

void Fighter::notify_collision(GameObject & object){
	//FIXME tá feio
	float floor_y = object.box.y + (box.x - object.box.x) * tan(object.rotation) - object.box.height * 0.5;
	if(object.is("floor") && speed.y >= 0 && not on_floor && abs(floor_y - (box.y + box.height * 0.5)) < 10){
		if(pass_through){
			if(object.is("platform")){
				if(((Floor&)object).get_id() == last_collided_floor)
					return;
				else
					pass_through = false;
			}
		}


		speed.y = 0;
		box.y = object.box.y + (box.x - object.box.x) * tan(object.rotation) - (box.height + object.box.height ) * 0.5;

		on_floor = true;
		last_collided_floor = ((Floor&)object).get_id();
		pass_through = false;
	}else if(object.is("player")){
		Fighter & fighter = (Fighter &) object;
		if(fighter.is_attacking()){
			int left = AttackDirection::ATK_LEFT * (fighter.box.x > box.x);
			int right = AttackDirection::ATK_RIGHT * (fighter.box.x <= box.x);
			int up = AttackDirection::ATK_UP * (fighter.box.y > box.y);
			int down = AttackDirection::ATK_DOWN * (fighter.box.y <= box.y);
			int position_mask = left | right | up | down;
			if(position_mask & fighter.get_attack_mask()){
				float damage = fighter.get_attack_damage() * ((state == FighterState::DEFENDING) ? 0.5 : 1);
				remaining_life -= damage;
				float increment_special = (fighter.get_attack_damage() / 3) * ((state == FighterState::DEFENDING) ? 0 : 1);
				special += increment_special;
				if(special > MAX_SPECIAL) special = MAX_SPECIAL;
				if(state != FighterState::DEFENDING) check_stunt();
			}
		}else if(is_attacking()){
			grab = true;
			special += attack_damage / 2;
			if(special > MAX_SPECIAL) special = MAX_SPECIAL;
		}
	}

	change_state(temporary_state);
}

void Fighter::render(){
	int x = box.get_draw_x();
	int y = box.get_draw_y();

	SDL_RendererFlip flip = (orientation == Orientation::LEFT) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	sprite[state].render(x, y, rotation, flip);
}

bool Fighter::is_dead(){
	return remaining_life <= 0;
}

float Fighter::get_remaining_life(){
	return remaining_life;
}

float Fighter::get_special(){
	return special;
}

//only use in the end of update
void Fighter::change_state(FighterState cstate){
	if(state == cstate) return;

	if(joystick_id == -1) printf("%d to %d\n", state, cstate);

	float old_width = box.width;
	float old_height = box.height;
	state = cstate;
	Vector csize;
	if(cstate == CROUCH or cstate == CROUCH_ATK) csize = crouching_size;
	else csize = not_crouching_size;
	float new_width = csize.x;
	float new_height = csize.y;

	sprite[state].restart_count(n_sprite_start);
	n_sprite_start = 0;

	//float x = box.x - (new_width - old_width) * 0.5;
	float y = box.y - (new_height - old_height) * 0.5;

	//box.x = x;
	//box.y = y;
	printf("x = %f\n", box.x);
	box = Rectangle(box.x, y, csize.x, csize.y);
}

void Fighter::test_limits(){
	//TODO Matar personagem ao cair do cenario
	if(box.x < box.width / 2) box.x = box.width / 2;
	if(box.x > 1280 - box.width / 2) box.x = 1280 - box.width / 2;
	if(box.y < 0){
		box.y = 0;
		pass_through = false;
	}

	if(box.y > 900){
		if(is("test")) box.y = -100;
		else remaining_life = 0;
		//Comentar linha acima e descomentar duas abaixo para não morrer ao cair
		//box.y = 0;
		//pass_through = false;
	}
}

Fighter::AttackDirection Fighter::get_attack_orientation(){
	return (orientation == Orientation::LEFT ? AttackDirection::ATK_LEFT : AttackDirection::ATK_RIGHT);
}

void Fighter::reset_position(float x, float y){
	box.x = x;
	box.y = y;
	speed.y = 0;
}

bool Fighter::is_attacking(){
	return attack_damage > 0;
}

float Fighter::get_attack_damage(){
	return attack_damage;
}

int Fighter::get_attack_mask(){
	return attack_mask;
}
