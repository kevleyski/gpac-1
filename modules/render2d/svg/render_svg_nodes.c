/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Copyright (c) Cyril Concolato 2004
 *					All rights reserved
 *
 *  This file is part of GPAC / SVG Rendering sub-project
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "../visualsurface2d.h"

#ifndef GPAC_DISABLE_SVG
#include "svg_stacks.h"

/* Sets all SVG Properties to their initial value 
   The properties are then updated when going down the tree
   TODO: Check that all properties are there */
static void SVGInitProperties(SVGStylingProperties *svg_props) 
{
	if (!svg_props) return;

	GF_SAFEALLOC(svg_props->fill, sizeof(SVG_Paint));
	svg_props->fill->paintType = SVG_PAINTTYPE_COLOR;
	GF_SAFEALLOC(svg_props->fill->color, sizeof(SVG_Color));
	svg_props->fill->color->colorType = SVG_COLORTYPE_RGBCOLOR;
	svg_props->fill->color->red = 0;
	svg_props->fill->color->green = 0;
	svg_props->fill->color->blue = 0;

	GF_SAFEALLOC(svg_props->fill_rule, sizeof(SVG_ClipFillRule));
	*svg_props->fill_rule = SVGFillRule_nonzero;

	GF_SAFEALLOC(svg_props->fill_opacity, sizeof(SVG_OpacityValue));
	svg_props->fill_opacity->type = SVGFLOAT_VALUE;
	svg_props->fill_opacity->value = FIX_ONE;
	
	GF_SAFEALLOC(svg_props->stroke, sizeof(SVG_Paint));
	svg_props->stroke->paintType = SVG_PAINTTYPE_NONE;
	GF_SAFEALLOC(svg_props->stroke->color, sizeof(SVG_Color));
	svg_props->stroke->color->colorType = SVG_COLORTYPE_RGBCOLOR;

	GF_SAFEALLOC(svg_props->stroke_opacity, sizeof(SVG_OpacityValue));
	svg_props->stroke_opacity->type = SVGFLOAT_VALUE;
	svg_props->stroke_opacity->value = FIX_ONE;

	GF_SAFEALLOC(svg_props->stroke_width, sizeof(SVG_StrokeWidthValue));
	svg_props->stroke_width->unitType = SVG_LENGTHTYPE_NUMBER;
	svg_props->stroke_width->number = FIX_ONE;

	GF_SAFEALLOC(svg_props->stroke_linecap, sizeof(SVG_StrokeLineCapValue));
	*(svg_props->stroke_linecap) = SVGStrokeLineCap_butt;
	GF_SAFEALLOC(svg_props->stroke_linejoin, sizeof(SVG_StrokeLineJoinValue));
	*(svg_props->stroke_linejoin) = SVGStrokeLineJoin_miter;

	GF_SAFEALLOC(svg_props->stroke_miterlimit, sizeof(SVGInheritableFloat));
	svg_props->stroke_miterlimit->type = SVGFLOAT_VALUE;
	svg_props->stroke_miterlimit->value = 4*FIX_ONE;

	GF_SAFEALLOC(svg_props->stroke_dashoffset , sizeof(SVG_StrokeDashOffsetValue));
	svg_props->stroke_dashoffset->type = SVGFLOAT_VALUE;
	svg_props->stroke_dashoffset->value = 0;

	GF_SAFEALLOC(svg_props->stroke_dasharray, sizeof(SVG_StrokeDashArrayValue));
	svg_props->stroke_dasharray->type = SVG_STROKEDASHARRAY_NONE;

	GF_SAFEALLOC(svg_props->font_family, sizeof(SVG_FontFamilyValue));
	svg_props->font_family->type = SVGFontFamily_string;
	svg_props->font_family->value.string = strdup("Arial");
	svg_props->font_family->value.length = 6;

	GF_SAFEALLOC(svg_props->font_size, sizeof(SVGInheritableFloat));
	svg_props->font_size->type = SVGFLOAT_VALUE;
	svg_props->font_size->value = 12*FIX_ONE;

	GF_SAFEALLOC(svg_props->font_style, sizeof(SVG_FontStyleValue));
	*(svg_props->font_style) = SVGFontStyle_normal;

	GF_SAFEALLOC(svg_props->color, sizeof(SVG_Color));
	svg_props->color->colorType = SVG_COLORTYPE_RGBCOLOR;
	/* svg_props->color->red, green, blue set to zero, so initial value for color property is black */

	GF_SAFEALLOC(svg_props->text_anchor, sizeof(SVG_TextAnchorValue));
	*svg_props->text_anchor = SVG_TEXTANCHOR_START;

	GF_SAFEALLOC(svg_props->visibility, sizeof(SVG_VisibilityValue));
	*svg_props->visibility = SVG_VISIBILITY_VISIBLE;

	GF_SAFEALLOC(svg_props->display, sizeof(SVG_DisplayValue));
	*svg_props->display = SVG_DISPLAY_INLINE;

}

static void SVGDeleteProperties(SVGStylingProperties **svg_props)
{
	if (!*svg_props) return;
	if((*svg_props)->fill) {
		free((*svg_props)->fill->color);
		free((*svg_props)->fill);
	}
	if((*svg_props)->fill_rule) free((*svg_props)->fill_rule);
	if((*svg_props)->fill_opacity) free((*svg_props)->fill_opacity);
	if((*svg_props)->stroke) {
		free((*svg_props)->stroke->color);
		free((*svg_props)->stroke);
	}
	if((*svg_props)->stroke_opacity) free((*svg_props)->stroke_opacity);
	if((*svg_props)->stroke_width) free((*svg_props)->stroke_width);
	if((*svg_props)->stroke_linecap) free((*svg_props)->stroke_linecap);
	if((*svg_props)->stroke_linejoin) free((*svg_props)->stroke_linejoin);
	if((*svg_props)->stroke_miterlimit) free((*svg_props)->stroke_miterlimit);
	if((*svg_props)->stroke_dashoffset) free((*svg_props)->stroke_dashoffset);
	if((*svg_props)->stroke_dasharray) {
		if ((*svg_props)->stroke_dasharray->array.count) free((*svg_props)->stroke_dasharray->array.vals);
		free((*svg_props)->stroke_dasharray);
	}
	if((*svg_props)->font_family) {
		if ((*svg_props)->font_family->value.string) free((*svg_props)->font_family->value.string);
		free((*svg_props)->font_family);
	}
	if((*svg_props)->font_size) free((*svg_props)->font_size);
	if((*svg_props)->font_style) free((*svg_props)->font_style);
	if((*svg_props)->color) free((*svg_props)->color);
	if((*svg_props)->text_anchor) free((*svg_props)->text_anchor);
	if((*svg_props)->visibility) free((*svg_props)->visibility);
	if((*svg_props)->display) free((*svg_props)->display);
	free((*svg_props));
	*svg_props = NULL;
}

/* Updates the SVG Styling Properties of the renderer (render_svg_props) with the properties
   of the current SVG element (current_svg_props). Only the properties in current_svg_props 
   with a value different than inherit are updated.
   This function implements inheritance. 
   TODO: Check if all properties are implemented */
void SVGApplyProperties(SVGStylingProperties *render_svg_props, SVGStylingProperties current_svg_props)
{
	if (!render_svg_props) return;

	if (current_svg_props.color && current_svg_props.color->colorType != SVG_COLORTYPE_INHERIT) {
		render_svg_props->color = current_svg_props.color;
	}
	if (current_svg_props.fill && current_svg_props.fill->paintType != SVG_PAINTTYPE_INHERIT) {
		render_svg_props->fill = current_svg_props.fill;
	}
	if (current_svg_props.fill_rule && *current_svg_props.fill_rule != SVGFillRule_inherit) {
		render_svg_props->fill_rule = current_svg_props.fill_rule;
	}
	if (current_svg_props.fill_opacity && current_svg_props.fill_opacity->type != SVGFLOAT_INHERIT) {
		render_svg_props->fill_opacity = current_svg_props.fill_opacity;
	}
	if (current_svg_props.stroke && current_svg_props.stroke->paintType != SVG_PAINTTYPE_INHERIT) {
		render_svg_props->stroke = current_svg_props.stroke;
	}
	if (current_svg_props.stroke_opacity && current_svg_props.stroke_opacity->type != SVGFLOAT_INHERIT) {
		render_svg_props->stroke_opacity = current_svg_props.stroke_opacity;
	}
	if (current_svg_props.stroke_width && current_svg_props.stroke_width->unitType != SVG_LENGTHTYPE_INHERIT) {
		render_svg_props->stroke_width = current_svg_props.stroke_width;
	}
	if (current_svg_props.stroke_miterlimit && current_svg_props.stroke_miterlimit->type != SVGFLOAT_INHERIT) {
		render_svg_props->stroke_miterlimit = current_svg_props.stroke_miterlimit;
	}
	if (current_svg_props.stroke_linecap && *current_svg_props.stroke_linecap != SVGStrokeLineCap_inherit) {
		render_svg_props->stroke_linecap = current_svg_props.stroke_linecap;
	}
	if (current_svg_props.stroke_linejoin && *current_svg_props.stroke_linejoin != SVGStrokeLineJoin_inherit) {
		render_svg_props->stroke_linejoin = current_svg_props.stroke_linejoin;
	}
	if (current_svg_props.stroke_dashoffset && current_svg_props.stroke_dashoffset->value != SVGFLOAT_INHERIT) {
		render_svg_props->stroke_dashoffset = current_svg_props.stroke_dashoffset;
	}
	if (current_svg_props.stroke_dasharray && current_svg_props.stroke_dasharray->type != SVG_STROKEDASHARRAY_INHERIT) {
		render_svg_props->stroke_dasharray = current_svg_props.stroke_dasharray;
	}
	if (current_svg_props.font_family && current_svg_props.font_family->type != SVGFontFamily_inherit) {
		render_svg_props->font_family = current_svg_props.font_family;
	}
	if (current_svg_props.font_size && current_svg_props.font_size->type != SVGFLOAT_INHERIT) {
		render_svg_props->font_size = current_svg_props.font_size;
	}
	if (current_svg_props.font_style && *current_svg_props.font_style != SVGFontStyle_inherit) {
		render_svg_props->font_style = current_svg_props.font_style;
	}
	if (current_svg_props.text_anchor && *current_svg_props.text_anchor != SVG_TEXTANCHOR_INHERIT) {
		render_svg_props->text_anchor = current_svg_props.text_anchor;
	}
	if (current_svg_props.visibility && *current_svg_props.visibility != SVG_VISIBILITY_INHERIT) {
		render_svg_props->visibility = current_svg_props.visibility;
	}
	if (current_svg_props.display && *current_svg_props.display != SVG_DISPLAY_INHERIT) {
		render_svg_props->display = current_svg_props.display;
	}
}

/* Set the viewport of the renderer based on the element that contains a viewport 
   TODO: change the SVGsvgElement into an element that has a viewport (more generic)
*/
static void SVGSetViewport(RenderEffect2D *eff, SVGsvgElement *svg) 
{
	GF_Matrix2D mat, tmp;
	Fixed real_width, real_height;

	gf_mx2d_init(mat);
	if (svg->width.unitType == SVG_LENGTHTYPE_NUMBER) 
		real_width = INT2FIX(eff->surface->render->compositor->scene_width);
	else
		/*u32 * fixed / u32*/
		real_width = eff->surface->render->compositor->scene_width*svg->width.number/100;

	if (svg->height.unitType == SVG_LENGTHTYPE_NUMBER)
		real_height = INT2FIX(eff->surface->render->compositor->scene_height);
	else 
		real_height = eff->surface->render->compositor->scene_height*svg->height.number/100;

	if (svg->viewBox.width != 0 && svg->viewBox.height != 0) {
		mat.m[0] = gf_divfix(real_width, svg->viewBox.width);
		mat.m[4] = gf_divfix(real_height, svg->viewBox.height);
		mat.m[2] = - gf_muldiv(svg->viewBox.x, real_width, svg->viewBox.width); 
		mat.m[5] = - gf_muldiv(svg->viewBox.y, real_height, svg->viewBox.height); 
	}
	gf_mx2d_copy(tmp, eff->transform);
	gf_mx2d_copy(eff->transform, mat);
	gf_mx2d_add_matrix(&eff->transform, &tmp);
}

/* Node specific rendering functions
   All the nodes follow the same principles:
	0) Check if the display property is not set to none, otherwise do not render
	1) Back-up of the renderer properties and apply the current ones
	2) Back-up of the coordinate system & apply geometric transformation if any
	3) Render the children if any or the shape if leaf node
	4) restore coordinate system
	5) restore styling properties
 */
static void SVG_Render_svg(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);
	SVGsvgElement *svg = (SVGsvgElement *)node;
	RenderEffect2D *eff = (RenderEffect2D *) rs;

	/* Exception for the SVG top node:
		before 1), initializes the styling properties and the geometric transformation */
	if (!eff->svg_props) {
		eff->svg_props = (SVGStylingProperties *) gf_node_get_private(node);
		/*To allow pan navigation*/
		eff->transform.m[5] *= -1;
	}

	/* 1) */
	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, svg->properties);	

	/* 0) */
	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE) return;

	/* 2) */
	gf_mx2d_copy(backup_matrix, eff->transform);
	SVGSetViewport(eff, svg);

	/* 3) */
	gf_node_render_children(node, eff);

	/* 4) */
	gf_mx2d_copy(eff->transform, backup_matrix);  

	/* 5) */
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Destroy_svg(GF_Node *node)
{
	SVGStylingProperties *svgp = (SVGStylingProperties *) gf_node_get_private(node);
	SVGDeleteProperties(&svgp);
}

void SVG_Init_svg(Render2D *sr, GF_Node *node)
{
	SVGStylingProperties *svgp;

	GF_SAFEALLOC(svgp, sizeof(SVGStylingProperties));
	SVGInitProperties(svgp);
	gf_node_set_private(node, svgp);

	gf_node_set_render_function(node, SVG_Render_svg);
	gf_node_set_predestroy_function(node, SVG_Destroy_svg);
}

static void SVG_Render_g(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	SVGgElement *g = (SVGgElement *)node;
	RenderEffect2D *eff = (RenderEffect2D *) rs;

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, g->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE) return;
	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(g->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	gf_node_render_children(node, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_g(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_g);
}

static void SVG_Render_rect(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGrectElement *rect = (SVGrectElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, rect->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(rect->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	/* 3) for a leaf node
	   Recreates the path (i.e the shape) only if the node is dirty 
	   (has changed compared to the previous rendering phase) */
	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		Fixed rx = rect->rx.number;
		Fixed ry = rect->ry.number;
		Fixed x = rect->x.number;
		Fixed y = rect->y.number;
		Fixed width = rect->width.number;
		Fixed height = rect->height.number;
		drawable_reset_path(cs);
		if (rx || ry) {
			if (rx >= width/2) rx = width/2;
			if (ry >= height/2) ry = height/2;
			if (rx == 0) rx = ry;
			if (ry == 0) ry = rx;
			gf_path_add_move_to(cs->path, x+rx, y);
			gf_path_add_line_to(cs->path, x+width-rx, y);
			gf_path_add_quadratic_to(cs->path, x+width, y, x+width, y+ry);
			gf_path_add_line_to(cs->path, x+width, y+height-ry);
			gf_path_add_quadratic_to(cs->path, x+width, y+height, x+width-rx, y+height);
			gf_path_add_line_to(cs->path, x+rx, y+height);
			gf_path_add_quadratic_to(cs->path, x, y+height, x, y+height-ry);
			gf_path_add_line_to(cs->path, x, y+ry);
			gf_path_add_quadratic_to(cs->path, x, y, x+rx, y);
			gf_path_close(cs->path);
		} else {
			gf_path_add_move_to(cs->path, x, y);
			gf_path_add_line_to(cs->path, x+width, y);		
			gf_path_add_line_to(cs->path, x+width, y+height);		
			gf_path_add_line_to(cs->path, x, y+height);		
			gf_path_close(cs->path);		
		}
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);
	/* end of 3) */

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_rect(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_rect);
}

static void SVG_Render_circle(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGcircleElement *circle = (SVGcircleElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, circle->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);
	tr = gf_list_get(circle->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		Fixed r = 2*circle->r.number;
		drawable_reset_path(cs);
		gf_path_add_ellipse(cs->path, circle->cx.number, circle->cy.number, r, r);
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_circle(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_circle);
}

static void SVG_Render_ellipse(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGellipseElement *ellipse = (SVGellipseElement *)node;

	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, ellipse->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(ellipse->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		drawable_reset_path(cs);
		gf_path_add_ellipse(cs->path, ellipse->cx.number, ellipse->cy.number, 2*ellipse->rx.number, 2*ellipse->ry.number);
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_ellipse(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_ellipse);
}

static void SVG_Render_line(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGlineElement *line = (SVGlineElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, line->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(line->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		drawable_reset_path(cs);
		gf_path_add_move_to(cs->path, line->x1.number, line->y1.number);
		gf_path_add_line_to(cs->path, line->x2.number, line->y2.number);
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_line(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_line);
}

static void SVG_Render_polyline(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGpolylineElement *polyline = (SVGpolylineElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, polyline->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(polyline->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		u32 i;
		u32 nbPoints = gf_list_count(polyline->points);
		drawable_reset_path(cs);
		if (nbPoints) {
			SVG_Point *p = gf_list_get(polyline->points, 0);
			gf_path_add_move_to(cs->path, p->x, p->y);
			for (i = 1; i < nbPoints; i++) {
				p = gf_list_get(polyline->points, i);
				gf_path_add_line_to(cs->path, p->x, p->y);
			}
			cs->node_changed = 1;
		} else {
			gf_path_add_move_to(cs->path, 0, 0);
		}
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_polyline(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_polyline);
}

static void SVG_Render_polygon(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGpolygonElement *polygon = (SVGpolygonElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, polygon->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(polygon->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		u32 i;
		u32 nbPoints = gf_list_count(polygon->points);
		drawable_reset_path(cs);
		if (nbPoints) {
			SVG_Point *p = gf_list_get(polygon->points, 0);
			gf_path_add_move_to(cs->path, p->x, p->y);
			for (i = 1; i < nbPoints; i++) {
				p = gf_list_get(polygon->points, i);
				gf_path_add_line_to(cs->path, p->x, p->y);
			}
			gf_path_close(cs->path);
			cs->node_changed = 1;
		} else {
			gf_path_add_move_to(cs->path, 0, 0);
		}
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_polygon(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_polygon);
}

static void SVG_Render_path(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGpathElement *path = (SVGpathElement *)node;
  
	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, path->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(path->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		u32 i, j;
		SVG_Point orig, ct_orig, ct_end, end, *tmp;
		drawable_reset_path(cs);

		if (*(eff->svg_props->fill_rule)==GF_PATH_FILL_ZERO_NONZERO) cs->path->flags |= GF_PATH_FILL_ZERO_NONZERO;

		/* TODO: update for elliptical arcs */		
		for (i=0, j=0; i<gf_list_count(path->d.path_commands); i++) {
			u8 *command = gf_list_get(path->d.path_commands, i);
			switch (*command) {
			case 0: /* Move To */
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&orig, tmp, sizeof(SVG_Point));
				gf_path_add_move_to(cs->path, orig.x, orig.y);
				j++;
				break;
			case 1: /* Line To */
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&end, tmp, sizeof(SVG_Point));
				gf_path_add_line_to(cs->path, end.x, end.y);
				memcpy(&orig, &end, sizeof(SVG_Point));
				j++;
				break;
			case 2: /* Curve To */
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&ct_orig, tmp, sizeof(SVG_Point));
				tmp = gf_list_get(path->d.path_points, j+1);
				memcpy(&ct_end, tmp, sizeof(SVG_Point));
				tmp = gf_list_get(path->d.path_points, j+2);
				memcpy(&end, tmp, sizeof(SVG_Point));				 
				gf_path_add_cubic_to(cs->path, ct_orig.x, ct_orig.y, ct_end.x, ct_end.y, end.x, end.y);
				memcpy(&ct_orig, &ct_end, sizeof(SVG_Point));				 
				memcpy(&orig, &end, sizeof(SVG_Point));				 
				j+=3;
				break;
			case 3: /* Next Curve To */
				ct_orig.x = 2*orig.x - ct_orig.x;
				ct_orig.y = 2*orig.y - ct_orig.y;
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&ct_end, tmp, sizeof(SVG_Point));
				tmp = gf_list_get(path->d.path_points, j+1);
				memcpy(&end, tmp, sizeof(SVG_Point));
				gf_path_add_cubic_to(cs->path, ct_orig.x, ct_orig.y, ct_end.x, ct_end.y, end.x, end.y);
				memcpy(&ct_orig, &ct_end, sizeof(SVG_Point));				 
				memcpy(&orig, &end, sizeof(SVG_Point));				 
				j+=2;
				break;
			case 4: /* Quadratic Curve To */
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&ct_orig, tmp, sizeof(SVG_Point));
				tmp = gf_list_get(path->d.path_points, j+1);
				memcpy(&end, tmp, sizeof(SVG_Point));
				gf_path_add_quadratic_to(cs->path, ct_orig.x, ct_orig.y, end.x, end.y);			
				memcpy(&orig, &end, sizeof(SVG_Point));				 
				j+=2;
				break;
			case 5: /* Next Quadratic Curve To */
				ct_orig.x = 2*orig.x - ct_orig.x;
				ct_orig.y = 2*orig.y - ct_orig.y;
				tmp = gf_list_get(path->d.path_points, j);
				memcpy(&end, tmp, sizeof(SVG_Point));
				gf_path_add_quadratic_to(cs->path, ct_orig.x, ct_orig.y, end.x, end.y);
				memcpy(&orig, &end, sizeof(SVG_Point));				 
				j++;
				break;
			case 6: /* Close */
				gf_path_close(cs->path);
				break;
			}
		}
		gf_node_dirty_clear(node, 0);
		cs->node_changed = 1;
	}
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_path(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_path);
}

/* TODO: use sets a viewport ? can do clipping ? */
static void SVG_Render_use(GF_Node *node, void *rs)
{
	GF_Matrix2D backup_matrix;
	SVG_Transform *tr;
	DrawableContext *ctx;
	Drawable *cs = gf_node_get_private(node);
	RenderEffect2D *eff = rs;
	SVGuseElement *use = (SVGuseElement *)node;
  	GF_Matrix2D tmp, translate;

	SVGStylingProperties backup_props;
	u32 styling_size = sizeof(SVGStylingProperties);

	memcpy(&backup_props, eff->svg_props, styling_size);
	SVGApplyProperties(eff->svg_props, use->properties);

	if (*(eff->svg_props->display) == SVG_DISPLAY_NONE ||
		*(eff->svg_props->visibility) == SVG_VISIBILITY_HIDDEN) return;

	gf_mx2d_copy(backup_matrix, eff->transform);

	tr = gf_list_get(use->transform, 0);
	if (tr) {
		gf_mx2d_copy(eff->transform, tr->matrix);
		gf_mx2d_add_matrix(&eff->transform, &backup_matrix);
	}

	gf_mx2d_init(translate);
	translate.m[2] = use->x.number;
	translate.m[5] = use->y.number;
	gf_mx2d_copy(tmp, eff->transform);
	gf_mx2d_copy(eff->transform, translate);
	gf_mx2d_add_matrix(&eff->transform, &tmp);

	gf_node_render((GF_Node *)use->xlink_href.target_element, eff);
	ctx = SVG_drawable_init_context(cs, eff);
	if (!ctx) return;
			
	drawctx_store_original_bounds(ctx);
	drawable_finalize_render(ctx, eff);

	gf_mx2d_copy(eff->transform, backup_matrix);  
	memcpy(eff->svg_props, &backup_props, styling_size);
}

void SVG_Init_use(Render2D *sr, GF_Node *node)
{
	BaseDrawStack2D(sr, node);
	gf_node_set_render_function(node, SVG_Render_use);
}

/* end of rendering of basic shapes */

/* Interactive SVG elements */

typedef struct
{
	GF_Node *owner;
	GF_Renderer *compositor;
	GROUPINGNODESTACK2D
	Bool enabled;
	SensorHandler hdl;
} SVG_Stack_a;

static void SVG_Destroy_a(GF_Node *n)
{
	SVG_Stack_a *st = (SVG_Stack_a*)gf_node_get_private(n);
	R2D_UnregisterSensor(st->compositor, &st->hdl);
	if (st->compositor->interaction_sensors) st->compositor->interaction_sensors--;
	DeleteGroupingNode2D((GroupingNode2D *)st);
	free(st);
}

static void SVG_Render_a(GF_Node *node, void *rs)
{
	SVG_Stack_a *st = (SVG_Stack_a *) gf_node_get_private(node);
	SVGaElement *a = (SVGaElement *) node;
	RenderEffect2D *eff = rs;

	if (a->display == SVG_DISPLAY_NONE) return;

	/*update enabled state*/
	if (gf_node_dirty_get(node) & GF_SG_NODE_DIRTY) {
		st->enabled = 1;
	}
	/*note we don't clear dirty flag, this is done in traversing*/
	group2d_traverse((GroupingNode2D*)st, a->children, eff);
}

static Bool SVG_IsEnabled_a(SensorHandler *sh)
{
	return 1;
}

static void SVG_OnUserEvent_a(SensorHandler *sh, UserEvent2D *ev, GF_Matrix2D *sensor_matrix)
{
	SVG_Stack_a *st;
	GF_Event evt;
	SVGaElement *a;
	if (ev->event_type != GF_EVT_LEFTUP) return;
	st = (SVG_Stack_a *) gf_node_get_private(sh->owner);
	a = (SVGaElement *) sh->owner;

#ifndef DANAE
	if (!st->compositor->user->EventProc) return;
#endif
	evt.type = GF_EVT_NAVIGATE;
	
	if (a->xlink_href.type == SVGIri_iri) {
		evt.navigate.to_url = a->xlink_href.iri;
		if (evt.navigate.to_url) {
#ifdef DANAE
			loadDanaeUrl(st->compositor->danae_session, a->xlink_href.iri);
#else
			st->compositor->user->EventProc(st->compositor->user->opaque, &evt);
#endif
		}
	} else {
		u32 tag = gf_node_get_tag((GF_Node *)a->xlink_href.target_element);
		if (tag == TAG_SVG_set ||
			tag == TAG_SVG_animate ||
			tag == TAG_SVG_animateColor ||
			tag == TAG_SVG_animateTransform ||
			tag == TAG_SVG_animateMotion || 
			tag == TAG_SVG_discard) {
			SVGsetElement *set = (SVGsetElement *)a->xlink_href.target_element;
			SMIL_BeginOrEndValue *begin;
			GF_SAFEALLOC(begin, sizeof(SMIL_BeginOrEndValue));
			begin->type = SMILBeginOrEnd_offset_value;
			begin->clock_value = gf_node_get_scene_time((GF_Node *)set);
			gf_list_insert(set->begin, begin, 0);
		}
	}
}

SensorHandler *SVG_GetHandler_a(GF_Node *n)
{
	SVG_Stack_a *st = (SVG_Stack_a *) gf_node_get_private(n);
	return &st->hdl;
}

void SVG_Init_a(Render2D *sr, GF_Node *node)
{
	SVG_Stack_a *stack;
	GF_SAFEALLOC(stack, sizeof(SVG_Stack_a))

	SetupGroupingNode2D((GroupingNode2D*)stack, sr, node);

	sr->compositor->interaction_sensors++;

	stack->hdl.IsEnabled = SVG_IsEnabled_a;
	stack->hdl.OnUserEvent = SVG_OnUserEvent_a;
	stack->hdl.owner = node;
	gf_node_set_private(node, stack);
	gf_node_set_predestroy_function(node, SVG_Destroy_a);
	gf_node_set_render_function(node, SVG_Render_a);
}
/* end of Interactive SVG elements */

#endif //GPAC_DISABLE_SVG
