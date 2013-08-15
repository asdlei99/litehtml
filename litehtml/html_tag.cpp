#include "html.h"
#include "html_tag.h"
#include "tokenizer.h"
#include "document.h"
#include "iterators.h"
#include "stylesheet.h"
#include "table.h"
#include <algorithm>
#include <locale>

litehtml::html_tag::html_tag(litehtml::document* doc) : litehtml::element(doc)
{
	m_overflow				= overflow_visible;
	m_box					= 0;
	m_second_pass			= false;
	m_text_align			= text_align_left;
	m_el_position			= element_position_static;
	m_display				= display_inline;
	m_vertical_align		= va_baseline;
	m_list_style_type		= list_style_type_none;
	m_list_style_position	= list_style_position_outside;
	m_float					= float_none;
	m_clear					= clear_none;
	m_font					= 0;
	m_font_size				= 0;
	m_white_space			= white_space_normal;
	m_lh_predefined			= false;
	m_line_height			= 0;
}

litehtml::html_tag::~html_tag()
{

}

bool litehtml::html_tag::appendChild( litehtml::element* el )
{
	if(el)
	{
		el->parent(this);
		m_children.push_back(el);
		return true;
	}
	return false;
}

const litehtml::tchar_t* litehtml::html_tag::get_tagName() const
{
	return m_tag.c_str();
}

void litehtml::html_tag::set_attr( const tchar_t* name, const tchar_t* val )
{
	if(name && val)
	{
		tstring s_val = name;
		std::locale lc = std::locale::global(std::locale::classic());
		for(size_t i = 0; i < s_val.length(); i++)
		{
			s_val[i] = std::tolower(s_val[i], lc);
		}
		m_attrs[s_val] = val;
	}
}

const litehtml::tchar_t* litehtml::html_tag::get_attr( const tchar_t* name, const tchar_t* def )
{
	string_map::const_iterator attr = m_attrs.find(name);
	if(attr != m_attrs.end())
	{
		return attr->second.c_str();
	}
	return def;
}

bool litehtml::html_tag::select( const tchar_t* selector )
{
	std::vector<tstring> tokens;
	tokenize(selector, tokens, _t(","));
	for(std::vector<tstring>::iterator i = tokens.begin(); i != tokens.end(); i++)
	{
		trim(*i);
		if(select_one(i->c_str()))
		{
			return true;
		}
	}
	return false;
}

bool litehtml::html_tag::select_one( const tstring& selector )
{
	std::vector<tstring> tokens;
	tokenize(selector, tokens, _t(" \t>+"), _t(" \t>+"), _t("[]"));

	css_element_selector sel;
	sel.parse(_t("div.hello#rt[rel=none][title]"));


/*
	tstring::size_type pos = sel.find_first_of(_t(".#["));

	if(sel.substr(0, m_tag.length()) == m_tag || sel[0] == _t('*') || !iswalpha(sel[0]))
	{
		std::find_first_of()
		tstring::size_type pos = sel.find_first_not_of()
		return true;
	}
*/

	return false;
}

void litehtml::html_tag::apply_stylesheet( const litehtml::css& stylesheet )
{
	for(litehtml::css_selector::vector::const_iterator sel = stylesheet.selectors().begin(); sel != stylesheet.selectors().end(); sel++)
	{
		int apply = select(*(*sel), false);

		if(apply)
		{
			used_selector::ptr us = new used_selector((*sel), false);
			if(apply == 2)
			{
				if(select(*(*sel), true))
				{
					m_style.combine(*((*sel)->m_style));
					us->m_used = true;
				}
			} else
			{
				m_style.combine(*((*sel)->m_style));
				us->m_used = true;
			}
			m_used_styles.push_back(us);
		}
	}

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->apply_stylesheet(stylesheet);
	}
}

void litehtml::html_tag::get_content_size( size& sz, int max_width )
{
	sz.height	= 0;
	if(m_display == display_block)
	{
		sz.width	= max_width;
	} else
	{
		sz.width	= 0;
	}
}

void litehtml::html_tag::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	if(m_display == display_none)
	{
		return;
	}

	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	draw_background(hdc, x, y, clip);

	if(m_overflow > overflow_visible)
	{
		m_doc->container()->set_clip(pos, true, true);
	}

	if(m_display == display_list_item && m_list_style_type != list_style_type_none)
	{
		list_marker lm;

		const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
		size img_size;
		if(list_image)
		{
			css::parse_css_url(list_image, lm.image);
			lm.baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			m_doc->container()->get_image_size(lm.image.c_str(), lm.baseurl, img_size);
		} else
		{
			lm.baseurl = 0;
		}


		int sz_font		= get_font_size();
		lm.pos.x		= pos.x;
		lm.pos.y		= pos.y		+ sz_font / 3;
		lm.pos.width	= sz_font	- sz_font * 2 / 3;
		lm.pos.height	= sz_font	- sz_font * 2 / 3;

		if(img_size.width && img_size.height)
		{
			if(lm.pos.y + img_size.height > pos.y + pos.height)
			{
				lm.pos.y = pos.y + pos.height - img_size.height;
			}
			if(img_size.width > lm.pos.width)
			{
				lm.pos.x -= img_size.width - lm.pos.width;
			}

			lm.pos.width	= img_size.width;
			lm.pos.height	= img_size.height;
		}
		if(m_list_style_position == list_style_position_outside)
		{
			lm.pos.x -= sz_font;
		}

		lm.color = get_color(_t("color"), true, web_color(0, 0, 0));
		lm.marker_type = m_list_style_type;
		m_doc->container()->draw_list_marker(hdc, lm);
	}

	if(pos.does_intersect(clip))
	{
		draw_content(hdc, pos);
	}

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* el = (*i);
		if(!el->skip() && el->in_normal_flow())
		{
			el->draw(hdc, pos.left(), pos.top(), clip);
		}
	}

	for(elements_vector::iterator i = m_absolutes.begin(); i != m_absolutes.end(); i++)
	{
		element* el = (*i);
		el->draw(hdc, pos.left(), pos.top(), clip);
	}

	if(m_overflow > overflow_visible)
	{
		m_doc->container()->del_clip();
	}
}

litehtml::uint_ptr litehtml::html_tag::get_font(font_metrics* fm)
{
	if(fm)
	{
		*fm = m_font_metrics;
	}
	return m_font;
}

const litehtml::tchar_t* litehtml::html_tag::get_style_property( const tchar_t* name, bool inherited, const tchar_t* def /*= 0*/ )
{
	const tchar_t* ret = m_style.get_property(name);
	bool pass_parent = false;
	if(m_parent)
	{
		if(ret && !t_strcasecmp(ret, _t("inherit")))
		{
			pass_parent = true;
		} else if(!ret && inherited)
		{
			pass_parent = true;
		}
	}
	if(pass_parent)
	{
		ret = m_parent->get_style_property(name, inherited, def);
	}

	if(!ret)
	{
		ret = def;
	}

	return ret;
}

void litehtml::html_tag::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
}

void litehtml::html_tag::parse_styles(bool is_reparse)
{
	m_id	= get_attr(_t("id"), _t(""));
	m_class	= get_attr(_t("class"), _t(""));

	const tchar_t* style = get_attr(_t("style"));

	if(style)
	{
		m_style.add(style, NULL);
	}

	init_font();

	m_el_position	= (element_position)	value_index(get_style_property(_t("position"),		false,	_t("static")),		element_position_strings,	element_position_fixed);
	m_text_align	= (text_align)			value_index(get_style_property(_t("text-align"),	true,	_t("left")),		text_align_strings,			text_align_left);
	m_overflow		= (overflow)			value_index(get_style_property(_t("overflow"),		false,	_t("visible")),	overflow_strings,			overflow_visible);
	m_white_space	= (white_space)			value_index(get_style_property(_t("white-space"),	true,	_t("normal")),		white_space_strings,		white_space_normal);
	m_display		= (style_display)		value_index(get_style_property(_t("display"),		false,	_t("inline")),		style_display_strings,		display_inline);

	const tchar_t* va	= get_style_property(_t("vertical-align"), true,	_t("baseline"));
	m_vertical_align = (vertical_align) value_index(va, vertical_align_strings, va_baseline);

	const tchar_t* fl	= get_style_property(_t("float"), false,	_t("none"));
	m_float = (element_float) value_index(fl, element_float_strings, float_none);

	m_clear = (element_clear) value_index(get_style_property(_t("clear"), false, _t("none")), element_clear_strings, clear_none);

	if((m_el_position == element_position_absolute || m_float != float_none) && m_display != display_none)
	{
		m_display = display_block;
	}

	m_css_width.fromString(		get_style_property(_t("width"),			false,	_t("auto")), _t("auto"));
	m_css_height.fromString(	get_style_property(_t("height"),			false,	_t("auto")), _t("auto"));

	m_doc->cvt_units(m_css_width,	m_font_size);
	m_doc->cvt_units(m_css_height,	m_font_size);

	m_css_min_width.fromString(		get_style_property(_t("min-width"),		false,	_t("0")));
	m_css_min_height.fromString(	get_style_property(_t("min-height"),	false,	_t("0")));

	m_css_max_width.fromString(		get_style_property(_t("max-width"),		false,	_t("none")),	_t("none"));
	m_css_max_height.fromString(	get_style_property(_t("max-height"),	false,	_t("none")),	_t("none"));
	
	m_doc->cvt_units(m_css_min_width,	m_font_size);
	m_doc->cvt_units(m_css_min_height,	m_font_size);

	m_css_left.fromString(		get_style_property(_t("left"),				false,	_t("auto")), _t("auto"));
	m_css_right.fromString(		get_style_property(_t("right"),				false,	_t("auto")), _t("auto"));
	m_css_top.fromString(		get_style_property(_t("top"),				false,	_t("auto")), _t("auto"));
	m_css_bottom.fromString(	get_style_property(_t("bottom"),			false,	_t("auto")), _t("auto"));

	m_doc->cvt_units(m_css_left,	m_font_size);
	m_doc->cvt_units(m_css_right,	m_font_size);
	m_doc->cvt_units(m_css_top,		m_font_size);
	m_doc->cvt_units(m_css_bottom,	m_font_size);

	m_css_margins.left.fromString(		get_style_property(_t("margin-left"),			false,	_t("0")), _t("auto"));
	m_css_margins.right.fromString(		get_style_property(_t("margin-right"),			false,	_t("0")), _t("auto"));
	m_css_margins.top.fromString(		get_style_property(_t("margin-top"),			false,	_t("0")), _t("auto"));
	m_css_margins.bottom.fromString(	get_style_property(_t("margin-bottom"),		false,	_t("0")), _t("auto"));

	m_css_padding.left.fromString(		get_style_property(_t("padding-left"),			false,	_t("0")), _t(""));
	m_css_padding.right.fromString(		get_style_property(_t("padding-right"),		false,	_t("0")), _t(""));
	m_css_padding.top.fromString(		get_style_property(_t("padding-top"),			false,	_t("0")), _t(""));
	m_css_padding.bottom.fromString(	get_style_property(_t("padding-bottom"),		false,	_t("0")), _t(""));

	m_css_borders.left.width.fromString(	get_style_property(_t("border-left-width"),	false,	_t("medium")), border_width_strings);
	m_css_borders.right.width.fromString(	get_style_property(_t("border-right-width"),	false,	_t("medium")), border_width_strings);
	m_css_borders.top.width.fromString(		get_style_property(_t("border-top-width"),		false,	_t("medium")), border_width_strings);
	m_css_borders.bottom.width.fromString(	get_style_property(_t("border-bottom-width"),	false,	_t("medium")), border_width_strings);

	m_css_borders.left.color = web_color::from_string(get_style_property(_t("border-left-color"),	false,	_t("")));
	m_css_borders.left.style = (border_style) value_index(get_style_property(_t("border-left-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.right.color = web_color::from_string(get_style_property(_t("border-right-color"),	false,	_t("")));
	m_css_borders.right.style = (border_style) value_index(get_style_property(_t("border-right-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.top.color = web_color::from_string(get_style_property(_t("border-top-color"),	false,	_t("")));
	m_css_borders.top.style = (border_style) value_index(get_style_property(_t("border-top-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.bottom.color = web_color::from_string(get_style_property(_t("border-bottom-color"),	false,	_t("")));
	m_css_borders.bottom.style = (border_style) value_index(get_style_property(_t("border-bottom-style"), false, _t("none")), border_style_strings, border_style_none);

	m_css_borders.radius.top_left_x.fromString(get_style_property(_t("border-top-left-radius-x"), false, _t("0")));
	m_css_borders.radius.top_left_y.fromString(get_style_property(_t("border-top-left-radius-y"), false, _t("0")));

	m_css_borders.radius.top_right_x.fromString(get_style_property(_t("border-top-right-radius-x"), false, _t("0")));
	m_css_borders.radius.top_right_y.fromString(get_style_property(_t("border-top-right-radius-y"), false, _t("0")));

	m_css_borders.radius.bottom_right_x.fromString(get_style_property(_t("border-bottom-right-radius-x"), false, _t("0")));
	m_css_borders.radius.bottom_right_y.fromString(get_style_property(_t("border-bottom-right-radius-y"), false, _t("0")));

	m_css_borders.radius.bottom_left_x.fromString(get_style_property(_t("border-bottom-left-radius-x"), false, _t("0")));
	m_css_borders.radius.bottom_left_y.fromString(get_style_property(_t("border-bottom-left-radius-y"), false, _t("0")));

	m_doc->cvt_units(m_css_borders.radius.bottom_left_x,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_left_y,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_x,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.bottom_right_y,			m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_left_x,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_left_y,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_right_x,				m_font_size);
	m_doc->cvt_units(m_css_borders.radius.top_right_y,				m_font_size);

	m_margins.left		= m_doc->cvt_units(m_css_margins.left,		m_font_size);
	m_margins.right		= m_doc->cvt_units(m_css_margins.right,		m_font_size);
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		m_font_size);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	m_font_size);

	m_padding.left		= m_doc->cvt_units(m_css_padding.left,		m_font_size);
	m_padding.right		= m_doc->cvt_units(m_css_padding.right,		m_font_size);
	m_padding.top		= m_doc->cvt_units(m_css_padding.top,		m_font_size);
	m_padding.bottom	= m_doc->cvt_units(m_css_padding.bottom,	m_font_size);

	m_borders.left		= m_doc->cvt_units(m_css_borders.left.width,	m_font_size);
	m_borders.right		= m_doc->cvt_units(m_css_borders.right.width,	m_font_size);
	m_borders.top		= m_doc->cvt_units(m_css_borders.top.width,		m_font_size);
	m_borders.bottom	= m_doc->cvt_units(m_css_borders.bottom.width,	m_font_size);

	css_length line_height;
	line_height.fromString(get_style_property(_t("line-height"),	true,	_t("normal")), _t("normal"));
	if(line_height.is_predefined())
	{
		m_line_height = m_font_metrics.height;
		m_lh_predefined = true;
	} else if(line_height.units() == css_units_none)
	{
		m_line_height = (int) (line_height.val() * m_font_size);
		m_lh_predefined = false;
	} else
	{
		m_line_height =  m_doc->cvt_units(line_height,	m_font_size, m_font_size);
		m_lh_predefined = false;
	}


	if(m_display == display_list_item)
	{
		const tchar_t* list_type = get_style_property(_t("list-style-type"), true, _t("disc"));
		m_list_style_type = (list_style_type) value_index(list_type, list_style_type_strings, list_style_type_disc);

		const tchar_t* list_pos = get_style_property(_t("list-style-position"), true, _t("outside"));
		m_list_style_position = (list_style_position) value_index(list_pos, list_style_position_strings, list_style_position_outside);

		const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
		if(list_image)
		{
			tstring url;
			css::parse_css_url(list_image, url);

			const tchar_t* list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			m_doc->container()->load_image(url.c_str(), list_image_baseurl);
		}

	}

	parse_background();

	if(!is_reparse)
	{
		for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
		{
			(*i)->parse_styles();
		}

		init();
	}
}

int litehtml::html_tag::render( int x, int y, int max_width )
{
	int parent_width = max_width;

	// restore margins after collapse
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		m_font_size);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	m_font_size);

	// reset auto margins
	if(m_css_margins.left.is_predefined())
	{
		m_margins.left = 0;
	}
	if(m_css_margins.right.is_predefined())
	{
		m_margins.right = 0;
	}

	m_pos.move_to(x, y);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_css_left.calc_percent(parent_width);
	}

	int ret_width = 0;

	int block_width = 0;

	if(m_display != display_table_cell)
	{
		block_width = m_css_width.calc_percent(parent_width);
	}

	if(block_width)
	{
		ret_width = max_width = block_width;
	} else
	{
		if(max_width)
		{
			max_width -= content_margins_left() + content_margins_right();
		}
	}

	m_floats.clear();
	m_boxes.clear();

	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		element* el = (*i);
		int rw = place_element(el, max_width);
		if(rw > ret_width)
		{
			ret_width = rw;
		}
	}

	m_pos.height = 0;

	finish_last_box(true);

	m_pos.width		= max_width;
	calc_outlines(parent_width);

	if(!m_boxes.empty())
	{
		if(!is_body())
		{
			if(collapse_top_margin())
			{
				m_margins.top = std::max(m_boxes.front()->top_margin(), m_margins.top);
			}
			if(collapse_bottom_margin())
			{
				m_margins.bottom = std::max(m_boxes.back()->bottom_margin(), m_margins.bottom);
			}
		}
		m_pos.height = m_boxes.back()->bottom() - m_boxes.back()->bottom_margin();
	}

	// add the floats height to the block height
	if(is_floats_holder())
	{
		int floats_height = get_floats_height();
		if(floats_height > m_pos.height)
		{
			m_pos.height = floats_height;
		}
	}

	// calculate the final position

	m_pos.move_to(x, y);
	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();
	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_doc->cvt_units(m_css_left, m_font_size, parent_width);
	}

	// TODO: Percents are incorrect
	int block_height = m_css_height.calc_percent(m_pos.height);

	if(block_height)
	{
		m_pos.height = block_height;
	}

	int min_height = m_css_min_height.calc_percent(m_pos.height);

	if(m_display == display_list_item)
	{
		const tchar_t* list_image = get_style_property(_t("list-style-image"), true, 0);
		if(list_image)
		{
			tstring url;
			css::parse_css_url(list_image, url);

			size sz;
			const tchar_t* list_image_baseurl = get_style_property(_t("list-style-image-baseurl"), true, 0);
			m_doc->container()->get_image_size(url.c_str(), list_image_baseurl, sz);
			if(min_height < sz.height)
			{
				min_height = sz.height;
			}
		}

	}

	if(min_height > m_pos.height)
	{
		m_pos.height = min_height;
	}

	ret_width += content_margins_left() + content_margins_right();

	// re-render the inline-block with new width
	if((	m_display == display_inline_block						|| 
			(m_float != float_none && m_css_width.is_predefined())	|| 
			m_el_position == element_position_absolute ) 

			&& ret_width < max_width && !m_second_pass)
	{
		m_second_pass = true;
		render(x, y, ret_width);
		m_second_pass = false;
		m_pos.width = ret_width - (content_margins_left() + content_margins_right());
	}

	int min_width = m_css_min_width.calc_percent(parent_width);
	if(min_width != 0)
	{
		if(min_width > m_pos.width)
		{
			m_pos.width = min_width;
			ret_width	= min_width;
		}
	}

	int abs_base_x	= 0;
	int abs_base_y	= 0;

	int abs_base_width	= m_pos.width;
	int abs_base_height	= m_pos.height;

	for(elements_vector::iterator abs_el = m_absolutes.begin(); abs_el != m_absolutes.end(); abs_el++)
	{
		element* el = (*abs_el);

		int left	= el->get_css_left().calc_percent(abs_base_width);
		int right	= m_pos.width - el->get_css_right().calc_percent(abs_base_width);
		int top		= el->get_css_top().calc_percent(abs_base_height);
		int bottom	= m_pos.height - el->get_css_bottom().calc_percent(abs_base_height);
		el->render(left, top, right - left);
		int add = 0;
		if(el->get_css_top().is_predefined())
		{
			add = m_padding.top;
		}
		if(el->get_css_bottom().is_predefined())
		{
			add = m_padding.bottom;
		}
		if(m_pos.height + add < el->top() + el->height())
		{
			m_pos.height = el->top() + el->height() - add;
		}
	}

	for(elements_vector::iterator abs_el = m_absolutes.begin(); abs_el != m_absolutes.end(); abs_el++)
	{
		element* el = (*abs_el);

		position parent_pos;
		el->m_parent->get_abs_position(parent_pos, this);

		int left	= el->get_css_left().calc_percent(abs_base_width);
		int right	= m_pos.width - el->get_css_right().calc_percent(abs_base_width);
		int top		= el->get_css_top().calc_percent(abs_base_height);
		int bottom	= m_pos.height - el->get_css_bottom().calc_percent(abs_base_height);

		bool need_render = false;

		if(el->get_css_left().is_predefined() && el->get_css_right().is_predefined())
		{
			el->m_pos.x = parent_pos.left() + el->content_margins_left();
		} else if(!el->get_css_left().is_predefined() && el->get_css_right().is_predefined())
		{
			el->m_pos.x = left - m_padding.left + el->content_margins_left();
		} else if(el->get_css_left().is_predefined() && !el->get_css_right().is_predefined())
		{
			el->m_pos.x = right - el->width() + el->content_margins_right() + m_padding.right;
		} else
		{
			el->m_pos.x		= left - m_padding.left + el->content_margins_left();
			el->m_pos.width	= (right + el->content_margins_right() + m_padding.right) - el->m_pos.x;
			need_render = true;
		}

		if(el->get_css_top().is_predefined() && el->get_css_bottom().is_predefined())
		{
			el->m_pos.y = parent_pos.top() + el->content_margins_top();
		} else if(!el->get_css_top().is_predefined() && el->get_css_bottom().is_predefined())
		{
			el->m_pos.y = top - m_padding.top + el->content_margins_top();
		} else if(el->get_css_top().is_predefined() && !el->get_css_bottom().is_predefined())
		{
			el->m_pos.y = bottom - el->height() + el->content_margins_bottom() + m_padding.bottom;
		} else
		{
			el->m_pos.y			= top - m_padding.top + el->content_margins_top();
			el->m_pos.height	= (bottom + el->content_margins_bottom() + m_padding.bottom) - el->m_pos.y;
			need_render = true;
		}

		if(need_render)
		{
			position pos = el->m_pos;
			el->render(el->left(), el->right(), el->width());
			el->m_pos = pos;
		}
	}

	return ret_width;
}

bool litehtml::html_tag::is_white_space()
{
	return false;
}

int litehtml::html_tag::get_font_size()
{
	return m_font_size;
}

int litehtml::html_tag::get_base_line()
{
	if(is_replaced())
	{
		return 0;
	}
	int bl = 0;
	if(!m_boxes.empty())
	{
		bl = m_boxes.back()->baseline() + content_margins_bottom();
	}
	return bl;
}

void litehtml::html_tag::init()
{
	for(elements_vector::iterator el = m_children.begin(); el < m_children.end(); el++)
	{
		 if((*el)->get_element_position() == element_position_absolute)
		 {
			 add_absolute((*el));
		 }
	}
}

int litehtml::html_tag::select(const css_selector& selector, bool apply_pseudo)
{
	int right_res = select(selector.m_right, apply_pseudo);
	if(!right_res)
	{
		return 0;
	}
	if(selector.m_left)
	{
		if(!m_parent)
		{
			return false;
		}
		switch(selector.m_combinator)
		{
		case combinator_descendant:
			{
				bool is_pseudo = false;
				element* res = find_ancestor(*selector.m_left, apply_pseudo, &is_pseudo);
				if(!res)
				{
					return 0;
				} else
				{
					if(is_pseudo)
					{
						right_res = 2;
					}
				}
			}
			break;
		case combinator_child:
			{
				int res = m_parent->select(*selector.m_left, apply_pseudo);
				if(!res)
				{
					return 0;
				} else
				{
					if(right_res != 2)
					{
						right_res = res;
					}
				}
			}
			break;
// TODO: add combinator_adjacent_sibling and combinator_general_sibling handling
		}
	}
	return right_res;
}

int litehtml::html_tag::select(const css_element_selector& selector, bool apply_pseudo)
{
	if(!selector.m_tag.empty() && selector.m_tag != _t("*"))
	{
		if(selector.m_tag != m_tag)
		{
			return false;
		}
	}

	bool pseudo_found = false;

	for(css_attribute_selector::map::const_iterator i = selector.m_attrs.begin(); i != selector.m_attrs.end(); i++)
	{
		const tchar_t* attr_value = get_attr(i->first.c_str());
		switch(i->second.condition)
		{
		case select_exists:
			if(!attr_value)
			{
				return 0;
			}
			break;
		case select_equal:
			if(!attr_value)
			{
				return 0;
			} else 
			{
				if(i->first == _t("class"))
				{
					string_vector tokens;
					tokenize(attr_value, tokens, _t(" "));
					bool found = false;
					for(string_vector::iterator str = tokens.begin(); str != tokens.end() && !found; str++)
					{
						if(*str == i->second.val)
						{
							found = true;
						}
					}
					if(!found)
					{
						return 0;
					}
				} else
				{
					if(i->second.val != attr_value)
					{
						return 0;
					}
				}
			}
			break;
		case select_contain_str:
			if(!attr_value)
			{
				return 0;
			} else if(!t_strstr(attr_value, i->second.val.c_str()))
			{
				return 0;
			}
			break;
		case select_start_str:
			if(!attr_value)
			{
				return 0;
			} else if(t_strncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				return 0;
			}
			break;
		case select_end_str:
			if(!attr_value)
			{
				return 0;
			} else if(t_strncmp(attr_value, i->second.val.c_str(), i->second.val.length()))
			{
				const tchar_t* s = attr_value + t_strlen(attr_value) - i->second.val.length() - 1;
				if(s < attr_value)
				{
					return 0;
				}
				if(i->second.val != s)
				{
					return 0;
				}
			}
			break;
		case select_pseudo_class:
			if(apply_pseudo)
			{
				if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), i->second.val) == m_pseudo_classes.end())
				{
					return 0;
				}
			} else
			{
				pseudo_found = true;
			}
			break;
		}
	}
	if(pseudo_found)
	{
		return 2;
	}
	return 1;
}

litehtml::element* litehtml::html_tag::find_ancestor( const css_selector& selector, bool apply_pseudo, bool* is_pseudo )
{
	if(!m_parent)
	{
		return 0;
	}
	int res = m_parent->select(selector, apply_pseudo);
	if(res)
	{
		if(is_pseudo)
		{
			if(res == 2)
			{
				*is_pseudo = true;
			} else
			{
				*is_pseudo = false;
			}
		}
		return m_parent;
	}
	return m_parent->find_ancestor(selector, apply_pseudo, is_pseudo);
}

int litehtml::html_tag::get_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);

				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				h = std::max(h, el_pos.bottom());
			}
		}
		return h;
	}
	int h = m_parent->get_floats_height();
	return h - m_pos.y;
}

int litehtml::html_tag::get_left_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);
				if(el->get_float() == float_left)
				{
					position el_pos;
					el->get_abs_position(el_pos, this);
					el_pos += el->m_margins;
					el_pos += el->m_padding;
					el_pos += el->m_borders;

					h = std::max(h, el_pos.bottom());
				}
			}
		}
		return h;
	}
	int h = m_parent->get_left_floats_height();
	return h - m_pos.y;
}

int litehtml::html_tag::get_right_floats_height() const
{
	if(is_floats_holder())
	{
		int h = 0;
		if(!m_floats.empty())
		{
			for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
			{
				element::ptr el = (*i);
				if(el->get_float() == float_right)
				{
					position el_pos;
					el->get_abs_position(el_pos, this);
					el_pos += el->m_margins;
					el_pos += el->m_padding;
					el_pos += el->m_borders;

					h = std::max(h, el_pos.bottom());
				}
			}
		}
		return h;
	}
	int h = m_parent->get_left_floats_height();
	return h - m_pos.y;
}

int litehtml::html_tag::get_line_left( int y ) const
{
	if(is_floats_holder())
	{
		int w = 0;
		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			if(el->get_float() == float_left)
			{
				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				if(y >= el_pos.top() && y < el_pos.bottom())
				{
					w = std::max(w, el_pos.right());
				}
			}
		}
		return w;
	}
	int w = m_parent->get_line_left(y + m_pos.y);
	if(w < 0)
	{
		w = 0;
	}
	return w - (w ? m_pos.x : 0);
}

int litehtml::html_tag::get_line_right( int y, int def_right ) const
{
	if(is_floats_holder())
	{
		int w = def_right;
		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			if(el->get_float() == float_right)
			{
				position el_pos;
				el->get_abs_position(el_pos, this);
				el_pos += el->m_margins;
				el_pos += el->m_padding;
				el_pos += el->m_borders;

				if(y >= el_pos.top() && y < el_pos.bottom())
				{
					w = std::min(w, el_pos.left());
				}
			}
		}
		return w;
	}
	int w = m_parent->get_line_right(y + m_pos.y, def_right + m_pos.x);
	return w - m_pos.x;
}

void litehtml::html_tag::fix_line_width( int max_width, element_float flt )
{
	if(!m_boxes.empty())
	{
		elements_vector els;
		m_boxes.back()->get_elements(els);
		bool was_cleared = false;
		if(!els.empty() && els.front()->get_clear() != clear_none)
		{
			if(els.front()->get_clear() == clear_both)
			{
				was_cleared = true;
			} else
			{
				if(	flt == float_left	&& els.front()->get_clear() == clear_left ||
					flt == float_right	&& els.front()->get_clear() == clear_right)
				{
					was_cleared = true;
				}
			}
		}

		if(!was_cleared)
		{
			m_boxes.pop_back();

			for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
			{
				place_element((*i), max_width);
			}
		} else
		{
			int line_top = 0;
			if(m_boxes.back()->get_type() == box_line)
			{
				line_top = m_boxes.back()->top();
			} else
			{
				line_top = m_boxes.back()->bottom();
			}
			int line_left	= get_line_left(line_top);
			int line_right	= get_line_right(line_top, max_width);

			elements_vector els;
			m_boxes.back()->new_width(line_left, line_right, els);
			for(elements_vector::iterator i = els.begin(); i != els.end(); i++)
			{
				place_element((*i), max_width);
			}
		}
	}
}

void litehtml::html_tag::add_float( element* el )
{
	if(is_floats_holder())
	{
		element_float fl = el->get_float();
		if(fl == float_left || fl == float_right)
		{
			m_floats.push_back(el);
		}
	} else
	{
		m_parent->add_float(el);
	}
}

int litehtml::html_tag::find_next_line_top( int top, int width, int def_right )
{
	if(is_floats_holder())
	{
		int new_top = top;
		int_vector points;

		for(elements_vector::const_iterator i = m_floats.begin(); i != m_floats.end(); i++)
		{
			element::ptr el = (*i);
			position el_pos;
			el->get_abs_position(el_pos, this);
			el_pos += el->m_margins;
			el_pos += el->m_padding;
			el_pos += el->m_borders;

			if(el_pos.top() >= top)
			{
				if(find(points.begin(), points.end(), el_pos.top()) == points.end())
				{
					points.push_back(el_pos.top());
				}
			}
			if(el_pos.bottom() >= top)
			{
				if(find(points.begin(), points.end(), el_pos.bottom()) == points.end())
				{
					points.push_back(el_pos.bottom());
				}
			}
		}
		if(!points.empty())
		{
			sort(points.begin(), points.end(), std::less<int>( ));
			new_top = points.back();

			for(int_vector::iterator i = points.begin(); i != points.end(); i++)
			{
				int pos_left	= get_line_left((*i));
				int pos_right	= get_line_right((*i), def_right);
				if(pos_right - pos_left >= width)
				{
					new_top = (*i);
					break;
				}
			}
		}
		return new_top;
	}
	int new_top = m_parent->find_next_line_top(top + m_pos.y, width, def_right + m_pos.x);
	return new_top - m_pos.y;
}

void litehtml::html_tag::parse_background()
{
	// parse background-color
	m_bg.m_color		= get_color(_t("background-color"), false, web_color(0, 0, 0, 0));

	// parse background-position
	const tchar_t* str = get_style_property(_t("background-position"), false, _t("0% 0%"));
	if(str)
	{
		string_vector res;
		tokenize(str, res, _t(" \t"));
		if(res.size() > 0)
		{
			if(res.size() == 1)
			{
				if( value_in_list(res[0].c_str(), _t("left;right;center")) )
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.set_value(50, css_units_percentage);
				} else if( value_in_list(res[0].c_str(), _t("top;bottom;center")) )
				{
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
					m_bg.m_position.x.set_value(50, css_units_percentage);
				} else
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.set_value(50, css_units_percentage);
				}
			} else
			{
				if(value_in_list(res[0].c_str(), _t("left;right")))
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				} else if(value_in_list(res[0].c_str(), _t("top;bottom")))
				{
					m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
				} else if(value_in_list(res[1].c_str(), _t("left;right")))
				{
					m_bg.m_position.x.fromString(res[1], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[0], _t("top;bottom;center"));
				}else if(value_in_list(res[1].c_str(), _t("top;bottom")))
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				} else
				{
					m_bg.m_position.x.fromString(res[0], _t("left;right;center"));
					m_bg.m_position.y.fromString(res[1], _t("top;bottom;center"));
				}
			}

			if(m_bg.m_position.x.is_predefined())
			{
				switch(m_bg.m_position.x.predef())
				{
				case 0:
					m_bg.m_position.x.set_value(0, css_units_percentage);
					break;
				case 1:
					m_bg.m_position.x.set_value(100, css_units_percentage);
					break;
				case 2:
					m_bg.m_position.x.set_value(50, css_units_percentage);
					break;
				}
			}
			if(m_bg.m_position.y.is_predefined())
			{
				switch(m_bg.m_position.y.predef())
				{
				case 0:
					m_bg.m_position.y.set_value(0, css_units_percentage);
					break;
				case 1:
					m_bg.m_position.y.set_value(100, css_units_percentage);
					break;
				case 2:
					m_bg.m_position.y.set_value(50, css_units_percentage);
					break;
				}
			}
		} else
		{
			m_bg.m_position.x.set_value(0, css_units_percentage);
			m_bg.m_position.y.set_value(0, css_units_percentage);
		}
	} else
	{
		m_bg.m_position.y.set_value(0, css_units_percentage);
		m_bg.m_position.x.set_value(0, css_units_percentage);
	}

	str = get_style_property(_t("background-size"), false, _t("auto"));
	if(str)
	{
		string_vector res;
		tokenize(str, res, _t(" \t"));
		if(!res.empty())
		{
			m_bg.m_position.width.fromString(res[0], background_size_strings);
			if(res.size() > 1)
			{
				m_bg.m_position.height.fromString(res[1], background_size_strings);
			} else
			{
				m_bg.m_position.height.predef(background_size_auto);
			}
		} else
		{
			m_bg.m_position.width.predef(background_size_auto);
			m_bg.m_position.height.predef(background_size_auto);
		}
	}

	m_doc->cvt_units(m_bg.m_position.x,			m_font_size);
	m_doc->cvt_units(m_bg.m_position.y,			m_font_size);
	m_doc->cvt_units(m_bg.m_position.width,		m_font_size);
	m_doc->cvt_units(m_bg.m_position.height,	m_font_size);

	// parse background_attachment
	m_bg.m_attachment = (background_attachment) value_index(
		get_style_property(_t("background-attachment"), false, _t("scroll")), 
		background_attachment_strings, 
		background_attachment_scroll);

	// parse background_attachment
	m_bg.m_repeat = (background_repeat) value_index(
		get_style_property(_t("background-repeat"), false, _t("repeat")), 
		background_repeat_strings, 
		background_repeat_repeat);

	// parse background_clip
	m_bg.m_clip = (background_box) value_index(
		get_style_property(_t("background-clip"), false, _t("border-box")), 
		background_box_strings, 
		background_box_border);

	// parse background_origin
	m_bg.m_origin = (background_box) value_index(
		get_style_property(_t("background-origin"), false, _t("padding-box")), 
		background_box_strings, 
		background_box_content);

	// parse background-image
	css::parse_css_url(get_style_property(_t("background-image"), false, _t("")), m_bg.m_image);
	m_bg.m_baseurl = get_style_property(_t("background-image-baseurl"), false, _t(""));

	if(!m_bg.m_image.empty())
	{
		m_doc->container()->load_image(m_bg.m_image.c_str(), m_bg.m_baseurl.empty() ? 0 : m_bg.m_baseurl.c_str());
	}
}

void litehtml::html_tag::add_absolute( element* el )
{
	if(m_display != display_inline && m_el_position != element_position_static)
	{
		m_absolutes.push_back(el);
	} else if(m_parent)
	{
		m_parent->add_absolute(el);
	}
}

void litehtml::html_tag::calc_outlines( int parent_width )
{
	m_padding.left	= m_css_padding.left.calc_percent(parent_width);
	m_padding.right	= m_css_padding.right.calc_percent(parent_width);

	m_borders.left	= m_css_borders.left.width.calc_percent(parent_width);
	m_borders.right	= m_css_borders.right.width.calc_percent(parent_width);

	m_margins.left	= m_css_margins.left.calc_percent(parent_width);
	m_margins.right	= m_css_margins.right.calc_percent(parent_width);

	m_margins.top		= m_css_margins.top.calc_percent(0);
	m_margins.bottom	= m_css_margins.bottom.calc_percent(0);

	if(m_display == display_block)
	{
		if(m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right;
			m_margins.left	= (parent_width - el_width) / 2;
			m_margins.right	= (parent_width - el_width) - m_margins.left;
		} else if(m_css_margins.left.is_predefined() && !m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.right;
			m_margins.left	= parent_width - el_width;
		} else if(!m_css_margins.left.is_predefined() && m_css_margins.right.is_predefined())
		{
			int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.left;
			m_margins.right	= parent_width - el_width;
		}
	}
}

void litehtml::html_tag::parse_attributes()
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->parse_attributes();
	}
}

void litehtml::html_tag::get_text( tstring& text )
{
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		(*i)->get_text(text);
	}
}

bool litehtml::html_tag::is_body()  const
{
	return false;
}

void litehtml::html_tag::set_data( const tchar_t* data )
{

}

litehtml::background litehtml::html_tag::get_background()
{
	return m_bg;
}

void litehtml::html_tag::get_inline_boxes( position::vector& boxes )
{
	litehtml::box* old_box = 0;
	position pos;
	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		element* el = (*iter);
		if(!el->skip())
		{
			if(el->m_box)
			{
				if(el->m_box != old_box)
				{
					if(old_box)
					{
						boxes.push_back(pos);
					}
					old_box		= el->m_box;
					pos.x		= el->left() + el->margin_left();
					pos.y		= el->top() - m_padding.top - m_borders.top;
					pos.width	= 0;
					pos.height	= 0;
				}
				pos.width	= el->right() - pos.x - el->margin_right() - el->margin_left();
				pos.height	= std::max(pos.height, el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom);
			} else if(el->get_display() == display_inline)
			{
				el->get_inline_boxes(boxes);
			}
		}
	}
	if(pos.width || pos.height)
	{
		boxes.push_back(pos);
	}
}

bool litehtml::html_tag::on_mouse_over( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = set_pseudo_class(_t("hover"), is_point_inside(x, y));

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip() && (*iter)->get_element_position() != element_position_absolute)
		{
			if((*iter)->on_mouse_over(x - m_pos.x, y - m_pos.y))
			{
				ret = true;
			}
		}
	}

	for(elements_vector::iterator iter = m_absolutes.begin(); iter != m_absolutes.end(); iter++)
	{
		if((*iter)->on_mouse_over(x - m_pos.x, y - m_pos.y))
		{
			ret = true;
		}
	}

	return ret;
}

bool litehtml::html_tag::find_styles_changes( position::vector& redraw_boxes, int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = false;
	bool apply = false;
	for (used_selector::vector::iterator iter = m_used_styles.begin(); iter != m_used_styles.end() && !apply; iter++)
	{
		int res = select(*((*iter)->m_selector), true);
		if(!(res && (*iter)->m_used || !res && !(*iter)->m_used))
		{
			apply = true;
		}
	}

	if(apply)
	{
		if(m_display == display_inline ||  m_display == display_table_row)
		{
			position::vector boxes;
			get_inline_boxes(boxes);
			for(position::vector::iterator pos = boxes.begin(); pos != boxes.end(); pos++)
			{
				pos->x	+= x;
				pos->y	+= y;
				redraw_boxes.push_back(*pos);
			}
		} else
		{
			position pos(x + left(), y + top(), width(), height());
			redraw_boxes.push_back(pos);
		}

		ret = true;
		m_style.clear();
		for (used_selector::vector::iterator iter = m_used_styles.begin(); iter != m_used_styles.end(); iter++)
		{
			if(select(*(*iter)->m_selector, true))
			{
				(*iter)->m_used = true;
				m_style.combine(*((*iter)->m_selector->m_style));
			} else
			{
				(*iter)->m_used = false;
			}
		}
		parse_styles(/*true*/);
	}
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		if(!(*i)->skip() && (*i)->get_element_position() != element_position_absolute)
		{
			if((*i)->find_styles_changes(redraw_boxes, x + m_pos.x, y + m_pos.y))
			{
				ret = true;
			}
		}
	}
	for(elements_vector::iterator i = m_absolutes.begin(); i != m_absolutes.end(); i++)
	{
		if((*i)->find_styles_changes(redraw_boxes, x + m_pos.x, y + m_pos.y))
		{
			ret = true;
		}
	}
	return ret;
}

bool litehtml::html_tag::on_mouse_leave()
{
	bool ret = false;

	if(set_pseudo_class(_t("hover"), false))
	{
		ret = true;
	}

	if(set_pseudo_class(_t("active"), false))
	{
		ret = true;
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip() && (*iter)->get_element_position() != element_position_absolute)
		{
			if((*iter)->on_mouse_leave())
			{
				ret = true;
			}
		}
	}

	for(elements_vector::iterator iter = m_absolutes.begin(); iter != m_absolutes.end(); iter++)
	{
		if((*iter)->on_mouse_leave())
		{
			ret = true;
		}
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_down( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = set_pseudo_class(_t("active"), is_point_inside(x, y));

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip() && (*iter)->get_element_position() != element_position_absolute)
		{
			if((*iter)->on_lbutton_down(x - m_pos.x, y - m_pos.y))
			{
				ret = true;
			}
		}
	}

	for(elements_vector::iterator iter = m_absolutes.begin(); iter != m_absolutes.end(); iter++)
	{
		if((*iter)->on_lbutton_down(x - m_pos.x, y - m_pos.y))
		{
			ret = true;
		}
	}

	return ret;
}

bool litehtml::html_tag::on_lbutton_up( int x, int y )
{
	if(m_display == display_inline_text)
	{
		return false;
	}

	bool ret = false;

	if(set_pseudo_class(_t("active"), false))
	{
		ret = true;
		on_click(x, y);
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		if(!(*iter)->skip() && (*iter)->get_element_position() != element_position_absolute)
		{
			if((*iter)->on_lbutton_up(x, y))
			{
				ret = true;
			}
		}
	}

	for(elements_vector::iterator iter = m_absolutes.begin(); iter != m_absolutes.end(); iter++)
	{
		if((*iter)->on_lbutton_up(x, y))
		{
			ret = true;
		}
	}

	return ret;
}

void litehtml::html_tag::on_click( int x, int y )
{

}

const litehtml::tchar_t* litehtml::html_tag::get_cursor()
{
	const tchar_t* ret = 0;

	if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), _t("hover")) != m_pseudo_classes.end())
	{
		ret = get_style_property(_t("cursor"), true, 0);
	}

	for(elements_vector::iterator iter = m_children.begin(); iter != m_children.end(); iter++)
	{
		const tchar_t* cursor = (*iter)->get_cursor();
		if(cursor)
		{
			ret = cursor;
		}
	}

	return ret;
}

static const int font_size_table[8][7] =
{
	{ 9,    9,     9,     9,    11,    14,    18},
	{ 9,    9,     9,    10,    12,    15,    20},
	{ 9,    9,     9,    11,    13,    17,    22},
	{ 9,    9,    10,    12,    14,    18,    24},
	{ 9,    9,    10,    13,    16,    20,    26},
	{ 9,    9,    11,    14,    17,    21,    28},
	{ 9,   10,    12,    15,    17,    23,    30},
	{ 9,   10,    13,    16,    18,    24,    32}
};


void litehtml::html_tag::init_font()
{
	// initialize font size
	const tchar_t* str = get_style_property(_t("font-size"), false, 0);

	int parent_sz = 0;
	int doc_font_size = m_doc->container()->get_default_font_size();
	if(m_parent)
	{
		parent_sz = m_parent->get_font_size();
	} else
	{
		parent_sz = doc_font_size;
	}


	if(!str)
	{
		m_font_size = parent_sz;
	} else
	{
		m_font_size = parent_sz;

		css_length sz;
		sz.fromString(str, font_size_strings);
		if(sz.is_predefined())
		{
			int idx_in_table = doc_font_size - 9;
			if(idx_in_table >= 0 && idx_in_table <= 7)
			{
				if(sz.predef() >= fontSize_xx_small && sz.predef() <= fontSize_xx_large)
				{
					m_font_size = font_size_table[idx_in_table][sz.predef()];
				} else
				{
					m_font_size = doc_font_size;
				}
			} else			
			{
				switch(sz.predef())
				{
				case fontSize_xx_small:
					m_font_size = doc_font_size * 3 / 5;
					break;
				case fontSize_x_small:
					m_font_size = doc_font_size * 3 / 4;
					break;
				case fontSize_small:
					m_font_size = doc_font_size * 8 / 9;
					break;
				case fontSize_large:
					m_font_size = doc_font_size * 6 / 5;
					break;
				case fontSize_x_large:
					m_font_size = doc_font_size * 3 / 2;
					break;
				case fontSize_xx_large:
					m_font_size = doc_font_size * 2;
					break;
				default:
					m_font_size = doc_font_size;
					break;
				}
			}
		} else
		{
			if(sz.units() == css_units_percentage)
			{
				m_font_size = sz.calc_percent(parent_sz);
			} else
			{
				m_font_size = m_doc->cvt_units(sz, parent_sz);
			}
		}
	}

	// initialize font
	const tchar_t* name			= get_style_property(_t("font-family"),		true,	_t("inherit"));
	const tchar_t* weight		= get_style_property(_t("font-weight"),		true,	_t("normal"));
	const tchar_t* style		= get_style_property(_t("font-style"),			true,	_t("normal"));
	const tchar_t* decoration	= get_style_property(_t("text-decoration"),	true,	_t("none"));

	m_font = m_doc->get_font(name, m_font_size, weight, style, decoration, &m_font_metrics);
}

bool litehtml::html_tag::is_break() const
{
	return false;
}

void litehtml::html_tag::set_tagName( const tchar_t* tag )
{
	tstring s_val = tag;
	std::locale lc = std::locale::global(std::locale::classic());
	for(size_t i = 0; i < s_val.length(); i++)
	{
		s_val[i] = std::tolower(s_val[i], lc);
	}
	m_tag = s_val;
}

void litehtml::html_tag::draw_background( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	position el_pos = pos;
	el_pos += m_padding;
	el_pos += m_borders;

	if(m_display != display_inline && m_display != display_table_row)
	{
		if(el_pos.does_intersect(clip))
		{
			background_paint bg_paint;
			init_background_paint(pos, bg_paint);

			m_doc->container()->draw_background(hdc, bg_paint);
			m_doc->container()->draw_borders(hdc, m_css_borders, bg_paint.border_box);
		}
	} else
	{
		position::vector boxes;
		get_inline_boxes(boxes);

		for(position::vector::iterator box = boxes.begin(); box != boxes.end(); box++)
		{
			box->x	+= x;
			box->y	+= y;

			if(box->does_intersect(clip))
			{
				background_paint bg_paint;
				position content_box = *box;
				content_box -= m_borders;
				content_box -= m_padding;

				init_background_paint(content_box, bg_paint);

				bg_paint.border_radius	= css_border_radius();

				if(box == boxes.begin())
				{
					bg_paint.border_radius.bottom_left_x	= m_css_borders.radius.bottom_left_x;
					bg_paint.border_radius.bottom_left_y	= m_css_borders.radius.bottom_left_y;
					bg_paint.border_radius.top_left_x		= m_css_borders.radius.top_left_x;
					bg_paint.border_radius.top_left_y		= m_css_borders.radius.top_left_y;
				}
				if(box == boxes.end() - 1)
				{
					bg_paint.border_radius.bottom_right_x	= m_css_borders.radius.bottom_right_x;
					bg_paint.border_radius.bottom_right_y	= m_css_borders.radius.bottom_right_y;
					bg_paint.border_radius.top_right_x		= m_css_borders.radius.top_right_x;
					bg_paint.border_radius.top_right_y		= m_css_borders.radius.top_right_y;
				}

				m_doc->container()->draw_background(hdc, bg_paint);

				css_borders bdr;
				bdr.top		= m_css_borders.top;
				bdr.bottom	= m_css_borders.bottom;
				if(box == boxes.begin())
				{
					bdr.left	= m_css_borders.left;
				}
				if(box == boxes.end() - 1)
				{
					bdr.right	= m_css_borders.right;
				}
				bdr.radius = bg_paint.border_radius;
				m_doc->container()->draw_borders(hdc, bdr, *box);
			}
		}
	}
}

int litehtml::html_tag::render_inline( element* container, int max_width )
{
	int ret_width = 0;
	int rw = 0;
	for(elements_vector::iterator i = m_children.begin(); i != m_children.end(); i++)
	{
		if((*i)->in_normal_flow())
		{
			rw = container->place_element( (*i), max_width );
			if(rw > ret_width)
			{
				ret_width = rw;
			}
		}
	}
	return ret_width;
}

int litehtml::html_tag::place_element( element* el, int max_width )
{
	if(!el->in_normal_flow()) return 0;

	if(el->get_display() == display_inline)
	{
		return el->render_inline(this, max_width);
	}

	int ret_width = 0;

	if(el->get_float() == float_left)
	{
		int line_top = 0;
		if(!m_boxes.empty())
		{
			if(m_boxes.back()->get_type() == box_line)
			{
				line_top = m_boxes.back()->top();
			} else
			{
				line_top = m_boxes.back()->bottom();
			}
		}
		line_top		= get_cleared_top(el->get_clear(), line_top);
		int line_left	= get_line_left(line_top);
		int line_right	= get_line_right(line_top, max_width);

		el->render(line_left, line_top, line_right);
		if(el->right() > line_right)
		{
			int new_top = find_next_line_top(el->top(), el->width(), max_width);
			el->m_pos.x = get_line_left(new_top) + el->content_margins_left();
			el->m_pos.y = new_top + el->content_margins_top();
		}
		add_float(el);
		fix_line_width(max_width, float_left);
		ret_width = el->right();
	} else if(el->get_float() == float_right)
	{
		int line_top = 0;
		if(!m_boxes.empty())
		{
			if(m_boxes.back()->get_type() == box_line)
			{
				line_top = m_boxes.back()->top();
			} else
			{
				line_top = m_boxes.back()->bottom();
			}
		}
		line_top		= get_cleared_top(el->get_clear(), line_top);
		int line_left	= get_line_left(line_top);
		int line_right	= get_line_right(line_top, max_width);

		el->render(0, line_top, line_right);
		if(line_left + el->width() > line_right)
		{
			int new_top = find_next_line_top(el->top(), el->width(), max_width);
			el->m_pos.x = get_line_right(new_top, line_right) - (el->width() - el->content_margins_left());
			el->m_pos.y = new_top + el->content_margins_top();
		} else
		{
			el->m_pos.x = line_right - el->width();
		}
		add_float(el);
		fix_line_width(max_width, float_right);

		line_left	= get_line_left(line_top);
		line_right	= get_line_right(line_top, max_width);

		ret_width = line_left + (max_width - line_right);
	} else
	{
		int line_top = 0;
		if(!m_boxes.empty())
		{
			line_top = m_boxes.back()->top();
		}
		int line_left	= get_line_left(line_top);
		int line_right	= get_line_right(line_top, max_width);

		switch(el->get_display())
		{
		case display_inline_block:
			ret_width = el->render(line_left, line_top, line_right);
			break;
		case display_block:		
			if(el->is_replaced() || el->is_floats_holder())
			{
				el->m_pos.width		= el->get_css_width().calc_percent(line_right - line_left);
				el->m_pos.height	= el->get_css_height().calc_percent(0);
				if(el->m_pos.width || el->m_pos.height)
				{
					el->calc_outlines(line_right - line_left);
				}
			}
			break;
		case display_inline_text:
			{
				litehtml::size sz;
				el->get_content_size(sz, line_right);
				el->m_pos = sz;
			}
			break;
		}

		bool add_box = true;
		if(!m_boxes.empty())
		{
			if(m_boxes.back()->can_hold(el, m_white_space))
			{
				add_box = false;
			}
		}
		if(add_box)
		{
			line_top = new_box(el, max_width);
		} else if(!m_boxes.empty())
		{
			line_top = m_boxes.back()->top();
		}

		line_left	= get_line_left(line_top);
		line_right	= get_line_right(line_top, max_width);

		if(!el->is_inline_box())
		{
			if(m_boxes.size() == 1)
			{
				if(collapse_top_margin())
				{
					int shift = el->margin_top();
					if(shift >= 0)
					{
						line_top -=shift;
						m_boxes.back()->y_shift(-shift);
					}
				}
			} else
			{
				int shift = 0;
				int prev_margin = m_boxes[m_boxes.size() - 2]->bottom_margin();

				if(prev_margin > el->margin_top())
				{
					shift = el->margin_top();
				} else
				{
					shift = prev_margin;
				}
				if(shift >= 0)
				{
					line_top -= shift;
					m_boxes.back()->y_shift(-shift);
				}
			}
		}

		switch(el->get_display())
		{
		case display_table:
		case display_list_item:
			ret_width = el->render(line_left, line_top, line_right - line_left);
			break;
		case display_block:
		case display_table_cell:
		case display_table_caption:
		case display_table_row:
			if(el->is_replaced() || el->is_floats_holder())
			{
				ret_width = el->render(line_left, line_top, line_right - line_left) + line_left;
			} else
			{
				ret_width = el->render(0, line_top, max_width);
			}
			break;
		}

		m_boxes.back()->add_element(el);

		if(el->is_inline_box() && !el->skip())
		{
			ret_width = el->right();
		}
	}

	return ret_width;
}

bool litehtml::html_tag::set_pseudo_class( const tchar_t* pclass, bool add )
{
	if(add)
	{
		if(std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), pclass) == m_pseudo_classes.end())
		{
			m_pseudo_classes.push_back(pclass);
			return true;
		}
	} else
	{
		string_vector::iterator pi = std::find(m_pseudo_classes.begin(), m_pseudo_classes.end(), pclass);
		if(pi != m_pseudo_classes.end())
		{
			m_pseudo_classes.erase(pi);
			return true;
		}
	}
	return false;
}

int litehtml::html_tag::line_height() const
{
	return m_line_height;
}

bool litehtml::html_tag::is_replaced() const
{
	return false;
}

int litehtml::html_tag::finish_last_box(bool end_of_render)
{
	int line_top = 0;

	if(!m_boxes.empty())
	{
		m_boxes.back()->finish(end_of_render);

		if(m_boxes.back()->is_empty())
		{
			line_top = m_boxes.back()->top();
			m_boxes.pop_back();
		}

		if(!m_boxes.empty())
		{
			line_top = m_boxes.back()->bottom();
		}
	}
	return line_top;
}

int litehtml::html_tag::new_box( element* el, int max_width )
{
	int line_top	= get_cleared_top(el->get_clear(), finish_last_box());

	int line_left	= get_line_left(line_top);
	int line_right	= get_line_right(line_top, max_width);

	if(el->is_inline_box() || el->is_floats_holder())
	{
		if(el->width() > line_right - line_left)
		{
			line_top	= find_next_line_top(line_top, el->width(), max_width);
			line_left	= get_line_left(line_top);
			line_right	= get_line_right(line_top, max_width);
		}
	}

	if(el->is_inline_box())
	{
		font_metrics fm = {0};
		get_font(&fm);
		line_box* lb = new line_box(line_top, line_left, line_right, line_height(), fm, m_text_align);
		m_boxes.push_back(lb);
	} else
	{
		block_box* bb = new block_box(line_top, line_left, line_right);
		m_boxes.push_back(bb);
	}

	return line_top;
}

int litehtml::html_tag::get_cleared_top( element_clear clear, int line_top )
{
	switch(clear)
	{
	case clear_left:
		{
			int fh = get_left_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	case clear_right:
		{
			int fh = get_right_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	case clear_both:
		{
			int fh = get_floats_height();
			if(fh && fh > line_top)
			{
				line_top = fh;
			}
		}
		break;
	}
	return line_top;
}

litehtml::style_display litehtml::html_tag::get_display() const
{
	return m_display;
}

litehtml::element_float litehtml::html_tag::get_float() const
{
	return m_float;
}

bool litehtml::html_tag::is_floats_holder() const
{
	if(	m_display == display_inline_block || 
		m_display == display_table_cell || 
		!m_parent || 
		is_body() || 
		m_float != float_none ||
		m_el_position == element_position_absolute ||
		m_overflow > overflow_visible)
	{
		return true;
	}
	return false;
}

bool litehtml::html_tag::is_first_child( const element* el )
{
	if(!m_children.empty())
	{
		if(el == m_children.front())
		{
			return true;
		}
	}
	return false;
}

bool litehtml::html_tag::is_last_child( const element* el )
{
	if(!m_children.empty())
	{
		if(el == m_children.back())
		{
			return true;
		}
	}
	return false;
}

litehtml::white_space litehtml::html_tag::get_white_space() const
{
	return m_white_space;
}

litehtml::vertical_align litehtml::html_tag::get_vertical_align() const
{
	return m_vertical_align;
}

litehtml::css_length litehtml::html_tag::get_css_left() const
{
	return m_css_left;
}

litehtml::css_length litehtml::html_tag::get_css_right() const
{
	return m_css_right;
}

litehtml::css_length litehtml::html_tag::get_css_top() const
{
	return m_css_top;
}

litehtml::css_length litehtml::html_tag::get_css_bottom() const
{
	return m_css_bottom;
}

litehtml::element_clear litehtml::html_tag::get_clear() const
{
	return m_clear;
}

litehtml::css_length litehtml::html_tag::get_css_width() const
{
	return m_css_width;
}

litehtml::css_length litehtml::html_tag::get_css_height() const
{
	return m_css_height;
}

size_t litehtml::html_tag::get_children_count() const
{
	return m_children.size();
}

litehtml::element::ptr litehtml::html_tag::get_child( int idx ) const
{
	return m_children[idx];
}

void litehtml::html_tag::set_css_width( css_length& w )
{
	m_css_width = w;
}

void litehtml::html_tag::apply_vertical_align()
{
	if(!m_boxes.empty())
	{
		int add = 0;
		int content_height	= m_boxes.back()->bottom();

		if(m_pos.height > content_height)
		{
			switch(m_vertical_align)
			{
			case va_middle:
				add = (m_pos.height - content_height) / 2;
				break;
			case va_bottom:
				add = m_pos.height - content_height;
				break;
			}
		}

		if(add)
		{
			for(size_t i = 0; i < m_boxes.size(); i++)
			{
				m_boxes[i]->y_shift(add);
			}
		}
	}
}

litehtml::element_position litehtml::html_tag::get_element_position() const
{
	return m_el_position;
}

void litehtml::html_tag::init_background_paint( position pos, background_paint &bg_paint )
{
	bg_paint = m_bg;
	position content_box	= pos;
	position padding_box	= pos;
	padding_box += m_padding;
	position border_box		= padding_box;
	border_box += m_borders;

	switch(m_bg.m_clip)
	{
	case litehtml::background_box_padding:
		bg_paint.clip_box = padding_box;
		break;
	case litehtml::background_box_content:
		bg_paint.clip_box = content_box;
		break;
	default:
		bg_paint.clip_box = border_box;
		break;
	}

	switch(m_bg.m_origin)
	{
	case litehtml::background_box_border:
		bg_paint.origin_box = border_box;
		break;
	case litehtml::background_box_content:
		bg_paint.origin_box = content_box;
		break;
	default:
		bg_paint.origin_box = padding_box;
		break;
	}

	if(!bg_paint.image.empty())
	{
		m_doc->container()->get_image_size(bg_paint.image.c_str(), bg_paint.baseurl.c_str(), bg_paint.image_size);
		if(bg_paint.image_size.width && bg_paint.image_size.height)
		{
			litehtml::size img_new_sz = bg_paint.image_size;
			double img_ar_width		= (double) bg_paint.image_size.width / (double) bg_paint.image_size.height;
			double img_ar_height	= (double) bg_paint.image_size.height / (double) bg_paint.image_size.width;


			if(m_bg.m_position.width.is_predefined())
			{
				switch(m_bg.m_position.width.predef())
				{
				case litehtml::background_size_contain:
					if( (int) ((double) bg_paint.origin_box.width * img_ar_height) <= bg_paint.origin_box.height )
					{
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height	= (int) ((double) bg_paint.origin_box.width * img_ar_height);
					} else
					{
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width	= (int) ((double) bg_paint.origin_box.height * img_ar_width);
					}
					break;
				case litehtml::background_size_cover:
					if( (int) ((double) bg_paint.origin_box.width * img_ar_height) >= bg_paint.origin_box.height )
					{
						img_new_sz.width = bg_paint.origin_box.width;
						img_new_sz.height	= (int) ((double) bg_paint.origin_box.width * img_ar_height);
					} else
					{
						img_new_sz.height = bg_paint.origin_box.height;
						img_new_sz.width	= (int) ((double) bg_paint.origin_box.height * img_ar_width);
					}
					break;
					break;
				case litehtml::background_size_auto:
					if(!m_bg.m_position.height.is_predefined())
					{
						img_new_sz.height	= m_bg.m_position.height.calc_percent(bg_paint.origin_box.height);
						img_new_sz.width	= (int) ((double) img_new_sz.height * img_ar_width);
					}
					break;
				}
			} else
			{
				img_new_sz.width = m_bg.m_position.width.calc_percent(bg_paint.origin_box.width);
				if(m_bg.m_position.height.is_predefined())
				{
					img_new_sz.height = (int) ((double) img_new_sz.width * img_ar_height);
				} else
				{
					img_new_sz.height = m_bg.m_position.height.calc_percent(bg_paint.origin_box.height);
				}
			}

			bg_paint.image_size = img_new_sz;
			bg_paint.position_x = bg_paint.origin_box.x + (int) m_bg.m_position.x.calc_percent(bg_paint.origin_box.width - bg_paint.image_size.width);
			bg_paint.position_y = bg_paint.origin_box.y + (int) m_bg.m_position.y.calc_percent(bg_paint.origin_box.height - bg_paint.image_size.height);
		}

	}
	bg_paint.border_radius	= m_css_borders.radius;
	bg_paint.border_box		= border_box;
}
