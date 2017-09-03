/* 
 */

/*

    Copyright (C) 2014 Ferrero Andrea

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.


 */

/*

    These files are distributed with PhotoFlow - http://aferrero2707.github.io/PhotoFlow/

 */

#ifndef PF_OPTIONS_H
#define PF_OPTIONS_H

#include <string>
#include <lcms2.h>
#include <glibmm.h>

namespace PF
{

enum display_profile_t
{
  PF_DISPLAY_PROF_sRGB = 0,
  PF_DISPLAY_PROF_SYSTEM = 1,
  PF_DISPLAY_PROF_CUSTOM = 2,
  PF_DISPLAY_PROF_MAX
};


  class Options
  {
    profile_type_t working_profile_type;
    TRC_type working_trc_type;
    Glib::ustring custom_working_profile_name;

    display_profile_t display_profile_type;
    Glib::ustring custom_display_profile_name;
    int display_profile_intent;
    bool display_profile_bpc;

    Glib::ustring last_visited_image_folder;
    Glib::ustring last_visited_preset_folder;
    Glib::ustring last_visited_icc_folder;

    int save_sidecar_files;
    int use_default_preset;

  public:
    Options();

    void set_working_profile_type(int t);
    profile_type_t get_working_profile_type() { return working_profile_type; }
    void set_working_trc_type(int t);
    TRC_type get_working_trc_type() { return working_trc_type; }
    void set_custom_working_profile_name( std::string n )
    {
      custom_working_profile_name = n;
    }
    std::string get_custom_working_profile_name() { return custom_working_profile_name; }


    void set_display_profile_type(int t);
    display_profile_t get_display_profile_type() { return display_profile_type; }

    void set_custom_display_profile_name( std::string n )
    {
      custom_display_profile_name = n;
    }
    std::string get_custom_display_profile_name() { return custom_display_profile_name; }

    void set_display_profile_intent( int n )
    {
      display_profile_intent = n;
    }
    int get_display_profile_intent() { return display_profile_intent; }

    void set_display_profile_bpc( bool n )
    {
      display_profile_bpc = n;
    }
    bool get_display_profile_bpc() { return display_profile_bpc; }

    // last visited folders
    void set_last_visited_image_folder( std::string f ) { last_visited_image_folder = f; }
    std::string get_last_visited_image_folder() { return last_visited_image_folder; }
    void set_last_visited_preset_folder( std::string f ) { last_visited_preset_folder = f; }
    std::string get_last_visited_preset_folder() { return last_visited_preset_folder; }
    void set_last_visited_icc_folder( std::string f ) { last_visited_icc_folder = f; }
    std::string get_last_visited_icc_folder() { return last_visited_icc_folder; }

    void set_save_sidecar_files(int val) { save_sidecar_files = val; }
    int get_save_sidecar_files() { return save_sidecar_files; }

    void set_apply_default_preset(int val) { use_default_preset = val; }
    int get_apply_default_preset() { return use_default_preset; }

    void load();
    void save();
  };

}


#endif 


