#include "Gui_ResultListDisplay.hpp"


Gui_ResultListDisplay::Gui_ResultListDisplay(sigc::signal<void,Glib::RefPtr<Gio::File>,int>& signal_update_result_map, ResultMap& result_map):
    Gui_ResultDisplayBaseClass(signal_update_result_map, result_map),
    m_save_selected(TEXT_SAVE_SELECTED_RESULT),m_save_all(TEXT_SAVE_ALL_RESULT),m_delete_selected(TEXT_DELETE_SELECTED_RESULT),
    m_file_writer(this)

   {

    set_tooltip_text(TOOLTIP_RESULT_LIST_SIMPL);
    m_label.set_markup(RESULT_LIST_TITLE);

    Gtk::TreeView::Column* tmp_col;

    int c=0;
    m_tree_view.append_column("ID", m_col_model.m_col_id);
    tmp_col = m_tree_view.get_column(c++);
    if(tmp_col)
      tmp_col->set_sort_column(m_col_model.m_col_id);

    m_tree_view.append_column("File name", m_col_model.m_col_name);
    tmp_col = m_tree_view.get_column(c++);
    if(tmp_col)
      tmp_col->set_sort_column(m_col_model.m_col_name);


    m_tree_view.append_column("#Objects", m_col_model.m_col_n_objects);
    tmp_col = m_tree_view.get_column(c++);
    if(tmp_col)
      tmp_col->set_sort_column(m_col_model.m_col_n_objects);

    m_tree_view.append_column("#Excluded", m_col_model.m_col_n_excluded);
    tmp_col = m_tree_view.get_column(c++);
    if(tmp_col)
      tmp_col->set_sort_column(m_col_model.m_col_n_excluded);


    ++c;
    Gtk::CellRendererText* renderer_name = Gtk::manage( new Gtk::CellRendererText() );
    renderer_name->property_editable() = true;
    Gtk::TreeView::Column* column = Gtk::manage( new Gtk::TreeView::Column("Comment"));
    column->pack_start(*renderer_name, false);
    column->add_attribute(renderer_name->property_text(), m_col_model.m_comment);
    m_tree_view.append_column(*column);
    column->set_sort_column(m_col_model.m_comment);
    renderer_name->signal_edited().connect(sigc::mem_fun(*this, &Gui_ResultListDisplay::on_edit_comment));

    Glib::PropertyProxy< Pango::AttrList > pango_attrs = renderer_name->property_attributes();
    Pango::Attribute attr = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD );
    Pango::AttrList my_list;
    my_list.insert(attr);
    pango_attrs.set_value(my_list);


    m_tree_view.append_column("Full Path", m_col_model.m_col_path);
    tmp_col = m_tree_view.get_column(c++);
    if(tmp_col)
      tmp_col->set_sort_column(m_col_model.m_col_path);






//    Gtk::CellRenderer *cell = m_tree_view.get_column_cell_renderer(1);
//    Gtk::CellRendererText* cell_text = (Gtk::CellRendererText*) cell;
//
//    Glib::PropertyProxy< Pango::AttrList > pango_attrs = cell_text->property_attributes();
//    Pango::Attribute attr = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD );
//    Pango::AttrList my_list;
//    my_list.insert(attr);
//    pango_attrs.set_value(my_list);


    m_ref_tree_model = Gtk::ListStore::create(m_col_model);
    m_tree_view.set_model(m_ref_tree_model);


    m_ref_tree_selection = m_tree_view.get_selection();
    m_ref_tree_selection ->set_mode(Gtk::SELECTION_MULTIPLE);

    m_hbox.pack_start(m_save_all,false,false);
    m_hbox.pack_start(m_save_selected,false,false);
    m_hbox.pack_start(m_delete_selected,false,false);

    m_save_all.signal_clicked().connect( sigc::mem_fun(*this,&Gui_ResultListDisplay::on_save_all) );
    m_save_selected.signal_clicked().connect( sigc::mem_fun(*this,&Gui_ResultListDisplay::on_save) );
    m_delete_selected.signal_clicked().connect( sigc::mem_fun(*this,&Gui_ResultListDisplay::on_delete) );
   }

void Gui_ResultListDisplay::updateView(Glib::RefPtr<Gio::File> file, int idx){
    type_children children = m_ref_tree_model->children();
    DEV_INFOS("Updating view");
    Gtk::TreeModel::Row row;

    row = *findWichRow(children,idx);



    if(row == children.end())
        row = *(m_ref_tree_model->append());

    const Result& res(m_result_map.getResultAt(idx));
    const bool NA (m_result_map.getIsNAAt(idx));

    int N = (int) res.getNValid();
    int N_excl = (int) res.size() - res.getNValid();
    row[m_col_model.m_col_id] = idx;
    row[m_col_model.m_col_name] = file->get_basename();
    row[m_col_model.m_col_path] = file->get_path();
    row[m_col_model.m_comment] = m_result_map.getCommentAt(idx);
    if(!NA){
        std::stringstream ss;
        ss <<N;
        row[m_col_model.m_col_n_objects] = ss.str();
        ss.str("");
        ss<<N_excl;
        row[m_col_model.m_col_n_excluded] = ss.str();
    }
    else{
        std::string str("NA");
        row[m_col_model.m_col_n_objects] = str;

        row[m_col_model.m_col_n_excluded] = str;
    }


    m_ref_tree_selection->unselect_all();
    m_ref_tree_selection->select(row);



    m_tree_view.scroll_to_row(Gtk::TreePath(row));

    m_file_writer.setInUpToDate(false);
}
type_children::iterator Gui_ResultListDisplay::findWichRow(type_children& children,int idx){
    for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter){
        Gtk::TreeModel::Row tmp_row = *iter;
        if (tmp_row[m_col_model.m_col_id] == idx ){
                return iter;
            }
    }
    return children.end();
}
void Gui_ResultListDisplay::on_save_all(){
    m_indeces_of_selected.clear();
    type_children children = m_ref_tree_model->children();
    for(type_children::iterator ite = children.begin();ite != children.end(); ++ite){
        m_indeces_of_selected.push_back((*ite)[m_col_model.m_col_id]);
        }
    m_file_writer.saveSelection(m_result_map,m_indeces_of_selected);
}
void Gui_ResultListDisplay::on_save(){
    m_indeces_of_selected.clear();
    m_ref_tree_selection->selected_foreach_iter( sigc::mem_fun(*this, &Gui_ResultListDisplay::makeIndecesOfSelected));
    m_file_writer.saveSelection(m_result_map,m_indeces_of_selected);
}
void Gui_ResultListDisplay::on_delete(){
    while(true){
        std::vector<Gtk::TreeModel::Path> rows = m_tree_view.get_selection() -> get_selected_rows();
        if (rows.empty())
            break;
        auto ite = m_ref_tree_model->get_iter(rows.back());
        m_ref_tree_model->erase(ite);
    }
}

void Gui_ResultListDisplay::on_edit_comment(const Glib::ustring& path, const Glib::ustring& text){
    Gtk::TreeModel::iterator iter = m_ref_tree_model->get_iter (path);
    if(iter){
        m_result_map.setCommentAt((*iter)[m_col_model.m_col_id],text);
        (*iter)[m_col_model.m_comment] = text;
    }

}
