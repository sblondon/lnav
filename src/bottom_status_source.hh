
#ifndef _bottom_status_source_hh
#define _bottom_status_source_hh

#include <string>

class bottom_status_source
    : public status_data_source,
      public grep_proc_control
{

public:

    typedef listview_curses::action::mem_functor_t<
	bottom_status_source> lv_functor_t;
    typedef textview_curses::action::mem_functor_t<
	bottom_status_source> tv_functor_t;
    
    typedef enum {
	BSF_LINE_NUMBER,
	BSF_PERCENT,
	BSF_HITS,
	BSF_WARNINGS,
	BSF_ERRORS,
	BSF_FILTERED,
	BSF_LOADING,

	BSF__MAX
    } field_t;
    
    bottom_status_source()
	: line_number_wire(*this, &bottom_status_source::update_line_number),
	  percent_wire(*this, &bottom_status_source::update_percent),
	  marks_wire(*this, &bottom_status_source::update_marks),
	  hits_wire(*this, &bottom_status_source::update_hits),
	  bss_error(80, view_colors::VCR_ALERT_STATUS),
	  bss_hit_spinner(0),
	  bss_load_percent(0) {
	this->bss_fields[BSF_LINE_NUMBER].set_width(8);
	this->bss_fields[BSF_PERCENT].set_width(4);
	this->bss_fields[BSF_HITS].set_width(16);
	this->bss_fields[BSF_HITS].set_cylon(true);
	this->bss_fields[BSF_WARNINGS].set_width(10);
	this->bss_fields[BSF_ERRORS].set_width(10);
	this->bss_fields[BSF_ERRORS].set_role(view_colors::VCR_ALERT_STATUS);
	this->bss_fields[BSF_FILTERED].set_width(14);
	this->bss_fields[BSF_LOADING].set_width(13);
	this->bss_fields[BSF_LOADING].set_cylon(true);
	this->bss_fields[BSF_LOADING].right_justify(true);
    };
    
    virtual ~bottom_status_source() { };

    lv_functor_t line_number_wire;
    lv_functor_t percent_wire;
    lv_functor_t marks_wire;
    tv_functor_t hits_wire;
    
    status_field &get_field(field_t id) { return this->bss_fields[id]; };

    void grep_error(std::string msg) {
	this->bss_error.set_value(msg);
    };

    size_t statusview_fields(void) {
	size_t retval;
	
	if (this->bss_error.empty())
	    retval = BSF__MAX;
	else
	    retval = 1;

	return retval;
    };
    
    status_field &statusview_value_for_field(int field) {
	if (this->bss_error.empty())
	    return this->get_field((field_t)field);
	else
	    return this->bss_error;
    };

    void update_line_number(listview_curses *lc) {
	status_field &sf = this->bss_fields[BSF_LINE_NUMBER];
	
	if (lc->get_inner_height() == 0)
	    sf.set_value("L0");
	else
	    sf.set_value("L%d", (int)lc->get_top());
    };

    void update_percent(listview_curses *lc) {
	status_field &sf = this->bss_fields[BSF_PERCENT];
	vis_line_t top = lc->get_top();
	vis_line_t bottom, height;
	unsigned long width;
	double percent;
	
	lc->get_dimensions(height, width);

	if (lc->get_inner_height() > 0) {
	    bottom = std::min(top + height - vis_line_t(1),
			      vis_line_t(lc->get_inner_height() - 1));
	    percent = (double)(bottom + 1);
	    percent /= (double)lc->get_inner_height();
	    percent *= 100.0;
	}
	else {
	    percent = 0.0;
	}
	sf.set_value("%3d%%", (int)percent);
    };

    void update_marks(listview_curses *lc) {
	textview_curses *tc = static_cast<textview_curses *>(lc);
	status_field &sfw = this->bss_fields[BSF_WARNINGS];
	status_field &sfe = this->bss_fields[BSF_ERRORS];
	bookmarks &bm = tc->get_bookmarks();
	unsigned long width;
	vis_line_t height;

	tc->get_dimensions(height, width);
	if (bm.find(&logfile_sub_source::BM_WARNINGS) != bm.end()) {
	    bookmark_vector &bv = bm[&logfile_sub_source::BM_WARNINGS];
	    bookmark_vector::iterator iter;

	    iter = lower_bound(bv.begin(), bv.end(), tc->get_top() + height);
	    sfw.set_value("%9dW", distance(iter, bv.end()));
	}
	else {
	    sfw.clear();
	}
	
	if (bm.find(&logfile_sub_source::BM_ERRORS) != bm.end()) {
	    bookmark_vector &bv = bm[&logfile_sub_source::BM_ERRORS];
	    bookmark_vector::iterator iter;

	    iter = lower_bound(bv.begin(), bv.end(), tc->get_top() + height);
	    sfe.set_value("%9dE", distance(iter, bv.end()));
	}
	else {
	    sfe.clear();
	}
    };

    void update_hits(textview_curses *tc) {
	status_field &sf = this->bss_fields[BSF_HITS];
	view_colors::role_t new_role;

	if (tc->is_searching()) {
	    this->bss_hit_spinner += 1;
	    if (this->bss_hit_spinner % 2)
		new_role = view_colors::VCR_ACTIVE_STATUS;
	    else
		new_role = view_colors::VCR_ACTIVE_STATUS2;
	    sf.set_cylon(true);
	}
	else {
	    new_role = view_colors::VCR_STATUS;
	    sf.set_cylon(false);
	}
	sf.set_role(new_role);
	this->bss_error.clear();
	sf.set_value("%9d hits", tc->get_match_count());
    };

    void update_loading(off_t off, size_t total) {
	status_field &sf = this->bss_fields[BSF_LOADING];
	
	if ((size_t)off == total) {
	    sf.set_role(view_colors::VCR_STATUS);
	    sf.clear();
	}
	else {
	    int pct = ((double)off / (double)total) * 100.0;
	    
	    if (this->bss_load_percent != pct) {

		this->bss_load_percent = pct;
	    
		sf.set_role(view_colors::VCR_ACTIVE_STATUS2);
		sf.set_value(" Loading %2d%% ", pct);
	    }
	}
    };

    void update_filtered(logfile_sub_source &lss) {
	status_field &sf = this->bss_fields[BSF_FILTERED];
	
	if (lss.get_filtered_count() == 0)
	    sf.clear();
	else
	    sf.set_value("%d Not Shown", lss.get_filtered_count());
    };

private:
    status_field bss_error;
    status_field bss_fields[BSF__MAX];
    int bss_hit_spinner;
    int bss_load_percent;
};

#endif