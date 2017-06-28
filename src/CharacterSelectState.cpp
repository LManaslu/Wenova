#include "CharacterSelectState.h"
#include "Game.h"
#include "MenuState.h"
#include "BattleState.h"

CharacterSelectState::CharacterSelectState(){
	background = Sprite("character_select/background.png");
	character_slots = Sprite("character_select/character_slots.png");

	// FIXME 2 enabled characters
	for(int i=0;i<2;i++){
		available_skin[get_char_info(i).first].assign(4, true);

		char_name[get_char_info(i).first] = Sprite("character_select/chars/" + get_char_info(i).first + "/name.png");

		// loop to get skins
		for(int j=0;j<4;j++){
			char_sprite[get_char_info(i).first].push_back(Sprite("character_select/chars/" + get_char_info(i).first + "/" + to_string(j) + ".png",
													get_char_info(i).second,
													13));
			char_sprite[get_char_info(i).first][j].set_scale_x(3);
			char_sprite[get_char_info(i).first][j].set_scale_y(3);
		}
	}

	for(int i=0;i<4;i++){
		name_tag[i] = Sprite("character_select/name_tag_" + to_string(i + 1) + ".png");
		number[i] = Sprite("character_select/number_" + to_string(i + 1) + ".png");
	}

	names = { {"flesh",""}, {"blood",""}, {"",""}, {"",""} };

	memset(cur_selection_col, 0, sizeof cur_selection_col);
	memset(cur_selection_row, 0, sizeof cur_selection_row);
	memset(cur_skin, 0, sizeof cur_skin);
	memset(selected, false, sizeof selected);

	name_tag_positions = { ii(91, 234), ii(92, 583), ii(956, 234), ii(955, 583) };
	number_delta = { ii(12, 9), ii(93, 9), ii(12, 101), ii(93, 101) };
	name_delta = { ii(118, 53), ii(117, 55), ii(47, 54), ii(50, 54) };
	sprite_pos = { ii(155, 32), ii(141, 379), ii(923, 34), ii(946, 381) };

	col_slots = { 510, 645 };
	row_slots = { 55, 197, 395, 536 };
}

void CharacterSelectState::update(float delta){
	InputManager * input_manager = InputManager::get_instance();

	// inputs
	if(input_manager->quit_requested() || input_manager->key_press(SDLK_ESCAPE) || input_manager->joystick_button_press(InputManager::B, 0)){
		m_quit_requested = true;
		Game::get_instance().push(new MenuState(true));
		return;
	}

	// only enable start when all players have selected a character
	if(all_players_selected()){
		if(input_manager->key_press(SDLK_RETURN) || input_manager->joystick_button_press(InputManager::START, 0)){
			m_quit_requested = true;
			Game::get_instance().push(new BattleState("1", "swamp_song.ogg"));
			return;
		}
	}

	for(int i=0;i<4;i++){
		if(selected[i]) continue;

		if((input_manager->key_press(SDLK_LEFT) || input_manager->joystick_button_press(InputManager::LEFT, i))
		&& cur_selection_col[i] != 0){
			if(character_enabled(cur_selection_row[i], cur_selection_col[i] - 1))
				cur_selection_col[i]--;
		}

		if((input_manager->key_press(SDLK_RIGHT) || input_manager->joystick_button_press(InputManager::RIGHT, i))
		&& cur_selection_col[i] != 1){
			if(character_enabled(cur_selection_row[i], cur_selection_col[i] + 1))
				cur_selection_col[i]++;
		}

		if((input_manager->key_press(SDLK_UP) || input_manager->joystick_button_press(InputManager::UP, i))
		&& cur_selection_row[i] != 0){
			if(character_enabled(cur_selection_row[i] - 1, cur_selection_col[i]))
				cur_selection_row[i]--;
		}

		if((input_manager->key_press(SDLK_DOWN) || input_manager->joystick_button_press(InputManager::DOWN, i))
		&& cur_selection_row[i] != 3){
			if(character_enabled(cur_selection_row[i] + 1, cur_selection_col[i]))
				cur_selection_row[i]++;
		}

		// skins
		if(input_manager->key_press(SDLK_COMMA) || input_manager->joystick_button_press(InputManager::LB, i)){
			cur_skin[i] = (cur_skin[i] - 1 + 4) % 4;
		}

		if(input_manager->key_press(SDLK_PERIOD) || input_manager->joystick_button_press(InputManager::RB, i)){
			cur_skin[i] = (cur_skin[i] + 1) % 4;
		}

		// select character && lock skin
		if(input_manager->key_press(SDLK_x) || input_manager->joystick_button_press(InputManager::A, i)){
			int col_sel = cur_selection_col[i];
			int row_sel = cur_selection_row[i];
			string char_selected = names[col_sel][row_sel];

			if(not available_skin[char_selected][cur_skin[i]]){
				printf("SKIN [%d] of [%s] ALREADY CHOSEN\n", cur_skin[i], char_selected.c_str());
			}
			else{
				printf("PLAYER %d CHOSE SKIN [%d] of [%s]\n", i + 1, cur_skin[i], char_selected.c_str());
				available_skin[char_selected][cur_skin[i]] = false;
				selected[i] = true;
			}
		}
	}

	for(auto it = char_sprite.begin(); it != char_sprite.end(); it++){
		for(int i=0; i < (*it).second.size(); i++){
			(*it).second[i].update(delta);
		}
	}
}

void CharacterSelectState::render(){
	background.render(0, 0);
	character_slots.render(0, 0);

	for(int i=0;i<4;i++){
		int col_sel = cur_selection_col[i];
		int row_sel = cur_selection_row[i];

		string char_selected = names[col_sel][row_sel];

		char_sprite[char_selected][cur_skin[i]].render(sprite_pos[i].first, sprite_pos[i].second);
		name_tag[i].render(name_tag_positions[i].first, name_tag_positions[i].second);

		char_name[char_selected].render(
			name_tag_positions[i].first + name_delta[i].first,
			name_tag_positions[i].second + name_delta[i].second
		);
		number[i].render(col_slots[col_sel] + number_delta[i].first, row_slots[row_sel] + number_delta[i].second);
	}
}

void CharacterSelectState::pause(){

}

void CharacterSelectState::resume(){

}

bool CharacterSelectState::character_enabled(int row, int){
	// Only characters in first row are available
	return row == 0;
}

// returns name and number of frames in corresponding sprite
pair<string, int> CharacterSelectState::get_char_info(int idx){
	switch(idx){
		case 0: return pair<string, int> ("flesh", 12);
		case 1: return pair<string, int> ("blood", 8);
	}
}

bool CharacterSelectState::all_players_selected(){
	for(auto cur : selected)
		if(not cur) return false;
	return true;
}