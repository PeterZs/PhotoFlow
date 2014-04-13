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

#ifndef IMAGE_EDITOR_H
#define IMAGE_EDITOR_H

#include <string>

#include <gtkmm.h>

#include "imagearea.hh"
#include "layerwidget.hh"


namespace PF {

  class ImageEditor: public Gtk::Paned
  {
    Image* image;

    Gtk::VBox imageBox;
    ImageArea imageArea;
    Gtk::ScrolledWindow imageArea_scrolledWindow;
    LayerWidget layersWidget;
    Gtk::HBox controlsBox;
    Gtk::Button buttonZoomIn, buttonZoomOut;
    Gtk::VBox radioBox;
    Gtk::RadioButton buttonShowMerged, buttonShowActive;

  public:
    ImageEditor( Image* image );
    ~ImageEditor();

    Image* get_image() { return image; }

    LayerWidget& get_layer_widget() { return layersWidget; }

    void zoom_in();
    void zoom_out();
  };

}

#endif
