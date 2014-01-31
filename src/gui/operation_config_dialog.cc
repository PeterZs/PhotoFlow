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

#include "operation_config_dialog.hh"


PF::OperationConfigDialog::OperationConfigDialog(const Glib::ustring&  	title):
  Gtk::Dialog(title, false, false),
  intensityAdj( 100, 0, 100, 1, 10, 0),
  opacityAdj( 100, 0, 100, 1, 10, 0),
  intensityScale(intensityAdj),
  opacityScale(opacityAdj)
{
  set_keep_above(true);
  add_button("OK",1);
  add_button("Cancel",0);

  signal_response().connect(sigc::mem_fun(*this,
					  &OperationConfigDialog::on_button_clicked) );

  lname.set_text( "name:" );
  nameBox.pack_start( lname, Gtk::PACK_SHRINK );

  nameEntry.set_text( "New Layer" );
  nameBox.pack_start( nameEntry, Gtk::PACK_SHRINK );

  blendmodeCombo.append("passthrough");
  blendmodeCombo.append("normal");
  blendmodeCombo.append("-----------");
  blendmodeCombo.append("overlay");
  blendmodeCombo.set_active( 1 );
  nameBox.pack_end( blendmodeCombo, Gtk::PACK_SHRINK );

  lblendmode.set_text( "blend mode:" );
  nameBox.pack_end( lblendmode, Gtk::PACK_SHRINK );

  topBox.pack_start( nameBox );

  lintensity.set_text( "intensity" );
  intensityScale.set_size_request(200, 30);
  intensityScale.set_digits(0);
  intensityScale.set_value_pos(Gtk::POS_RIGHT);
  lopacity.set_text( "opacity" );
  opacityScale.set_size_request(200, 30);
  opacityScale.set_digits(0);
  opacityScale.set_value_pos(Gtk::POS_RIGHT);

  lintensityAl.set(0,0.5,0,1);
  lintensityAl.add( lintensity );
  lopacityAl.set(0,0.5,0,1);
  lopacityAl.add( lopacity );

  controlsBoxLeft.pack_start( lintensityAl );
  controlsBoxLeft.pack_start( intensityScale );
  controlsBoxLeft.pack_start( lopacityAl );
  controlsBoxLeft.pack_start( opacityScale );
  controlsBox.pack_start( controlsBoxLeft, Gtk::PACK_SHRINK );
  topBox.pack_start( controlsBox );

  mainBox.pack_start( topBox );

  get_vbox()->pack_start( mainBox );


  intensityAdj.signal_value_changed().
    connect(sigc::mem_fun(*this,
			  &OperationConfigDialog::on_intensity_value_changed));

  opacityAdj.signal_value_changed().
    connect(sigc::mem_fun(*this,
			  &OperationConfigDialog::on_opacity_value_changed));


  // nameEntry.show();
  // nameBox.show();
  // topBox.show();
  // mainBox.show();

  show_all_children();
}


PF::OperationConfigDialog::~OperationConfigDialog()
{
}


void PF::OperationConfigDialog::add_widget( Gtk::Widget& widget )
{
  mainBox.pack_start( widget );

  show_all_children();
}


void PF::OperationConfigDialog::open()
{
  if( get_image() && get_layer() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    PF::OpParBase* par = get_layer()->get_processor()->get_par();
    intensityAdj.set_value( par->get_intensity()*100 );
    opacityAdj.set_value( par->get_opacity()*100 );
  }
  PF::OperationConfigUI::open();
  show_all();
}


void PF::OperationConfigDialog::on_button_clicked(int id)
{
  switch(id) {
  case 0:
    hide_all();
    break;
  case 1:
    hide_all();
    break;
  }
}


void PF::OperationConfigDialog::on_intensity_value_changed()
{
  double val = intensityAdj.get_value();
  std::cout<<"New intensity value: "<<val<<std::endl;
  if( get_image() && get_layer() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    get_layer()->get_processor()->get_par()->set_intensity( val/100. );
    get_layer()->set_dirty( true );
    std::cout<<"  updating image"<<std::endl;
    get_image()->update();
  }
}


void PF::OperationConfigDialog::on_opacity_value_changed()
{
  double val = opacityAdj.get_value();
  std::cout<<"New opacity value: "<<val<<std::endl;
  if( get_image() && get_layer() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {
    get_layer()->get_processor()->get_par()->set_opacity( val/100. );
    get_layer()->set_dirty( true );
    std::cout<<"  updating image"<<std::endl;
    get_image()->update();
  }
}
