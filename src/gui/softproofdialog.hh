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

#ifndef SOFTPROOF_DIALOG__HH
#define SOFTPROOF_DIALOG__HH

#include <gtkmm.h>


namespace PF {

  class ImageEditor;

  class SoftProofDialog: public Gtk::Dialog
  {
    //Tree model columns:
    class DCMModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:

      Gtk::TreeModelColumn<int> col_id;
      Gtk::TreeModelColumn<Glib::ustring> col_text;
      Gtk::TreeModelColumn<std::string> col_value;

      DCMModelColumns()
      { add(col_id); add(col_text); add(col_value); }
    };

    DCMModelColumns profile_columns;
    Glib::RefPtr<Gtk::ListStore> profile_model;
    Gtk::ComboBox profile_selector;

    DCMModelColumns intent_columns;
    Glib::RefPtr<Gtk::ListStore> intent_model;
    Gtk::ComboBox intent_selector;

    Gtk::Frame proofed_profile_frame;
    Gtk::VBox proofed_profile_box;

    Gtk::CheckButton bpc_button;
    Gtk::Label adaptation_label;
    Gtk::HScale adaptation_slider;


    Gtk::Frame display_profile_frame;
    Gtk::VBox display_profile_box;

    Gtk::Frame paper_sim_frame;
    Gtk::VBox paper_sim_vbox;
    Gtk::CheckButton sim_black_ink_button;
    Gtk::CheckButton sim_paper_color_button;

    Gtk::Frame clipping_frame;
    Gtk::HBox clipping_hbox;
    Gtk::CheckButton clip_negative_button;
    Gtk::CheckButton clip_overflow_button;

    Gtk::HBox sim_clipping_hbox;

    Gtk::CheckButton gamut_warning_button;

    Gtk::Image profile_open_img;
    Gtk::Button profile_open_button;
    Gtk::Entry profile_entry;
    Gtk::HBox profile_box;

    std::string custom_profile_name;

    ImageEditor* editor;
  public:
    SoftProofDialog(ImageEditor* editor);
    virtual ~SoftProofDialog();

    void on_button_clicked(int id);

    void open();

    void on_show();
    void on_hide();

    bool on_delete_event( GdkEventAny *   any_event );

    bool update_profile();
    bool update_intent();
    bool update_bpc();
    bool update_adaptation();
    bool update_sim_black_ink();
    bool update_sim_paper_color();
    bool update_clip_negative();
    bool update_clip_overflow();
    bool update_gamut_warning();

    void on_profile_selector_changed();
    void on_intent_selector_changed();
    void on_sim_black_ink_toggled();
    void on_bpc_toggled();
    void on_adaptation_changed();
    void on_clip_negative_toggled();
    void on_clip_overflow_toggled();
    void on_sim_paper_color_toggled();
    void on_gamut_warning_toggled();
    void on_button_profile_open_clicked();
  };

}


#endif
