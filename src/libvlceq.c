/*****************************************************************************
 * equalizer_presets.h:
 *****************************************************************************
 * Copyright (C) 2004 the VideoLAN team
 * $Id: 0c0c81215b776d1d5ab494c6f7a2f0327d1ee19b $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Equalizer presets
 *****************************************************************************/
/* Equalizer presets values are in this file instead of equalizer.c, so you can
 * get these values even if the equalizer is not enabled.
 */



#include "common.h"


#if (USEINTERNALEQ)


#include <input_internal.h>



#define N_ 


struct libvlc_equalizer_t
{
    double f_preamp;
    double f_amp[EQZ_BANDS_MAX];
};


static const double f_vlc_frequency_table_10b[EQZ_BANDS_MAX] =
{
    60.0, 170.0, 310.0, 600.0, 1000.0, 3000.0, 6000.0, 12000.0, 14000.0, 16000.0,
};


#if 0
static const double f_iso_frequency_table_10b[EQZ_BANDS_MAX] =
{
    31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000,
};


static const char *const preset_list[NB_PRESETS] = {
    "flat", "classical", "club", "dance", "fullbass", "fullbasstreble",
    "fulltreble", "headphones","largehall", "live", "party", "pop", "reggae",
    "rock", "ska", "soft", "softrock", "techno"
};
#endif

static const char *const preset_list_text[NB_PRESETS] = {
    N_("Flat"), N_("Classical"), N_("Club"), N_("Dance"), N_("Full bass"),
    N_("Full bass and treble"), N_("Full treble"), N_("Headphones"),
    N_("Large Hall"), N_("Live"), N_("Party"), N_("Pop"), N_("Reggae"),
    N_("Rock"), N_("Ska"), N_("Soft"), N_("Soft rock"), N_("Techno"),
};


typedef struct
{
    const char psz_name[16];
    int  i_band;
    double f_preamp;
    double f_amp[EQZ_BANDS_MAX];
} eqz_preset_t;


static const eqz_preset_t eqz_preset_10b[NB_PRESETS] =
{
    {
        "flat", 10, 12.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
    },
    {
        "classical", 10, 12.0,
        { -1.11022e-15, -1.11022e-15, -1.11022e-15, -1.11022e-15,
          -1.11022e-15, -1.11022e-15, -7.2, -7.2, -7.2, -9.6 }
    },
    {
        "club", 10, 6.0,
        { -1.11022e-15, -1.11022e-15, 8, 5.6, 5.6, 5.6, 3.2, -1.11022e-15,
          -1.11022e-15, -1.11022e-15 }
    },
    {
        "dance", 10, 5.0,
        { 9.6, 7.2, 2.4, -1.11022e-15, -1.11022e-15, -5.6, -7.2, -7.2,
          -1.11022e-15, -1.11022e-15 }
    },
    {
        "fullbass", 10, 5.0,
        { -8, 9.6, 9.6, 5.6, 1.6, -4, -8, -10.4, -11.2, -11.2  }
    },
    {
        "fullbasstreble", 10, 4.0,
        { 7.2, 5.6, -1.11022e-15, -7.2, -4.8, 1.6, 8, 11.2, 12, 12 }
    },
    {
        "fulltreble", 10, 3.0,
        { -9.6, -9.6, -9.6, -4, 2.4, 11.2, 16, 16, 16, 16.8 }
    },
    {
        "headphones", 10, 4.0,
        { 4.8, 11.2, 5.6, -3.2, -2.4, 1.6, 4.8, 9.6, 12.8, 14.4 }
    },
    {
        "largehall", 10, 5.0,
        { 10.4, 10.4, 5.6, 5.6, -1.11022e-15, -4.8, -4.8, -4.8, -1.11022e-15,
          -1.11022e-15 }
    },
    {
        "live", 10, 7.0,
        { -4.8, -1.11022e-15, 4, 5.6, 5.6, 5.6, 4, 2.4, 2.4, 2.4 }
    },
    {
        "party", 10, 6.0,
        { 7.2, 7.2, -1.11022e-15, -1.11022e-15, -1.11022e-15, -1.11022e-15,
          -1.11022e-15, -1.11022e-15, 7.2, 7.2 }
    },
    {
        "pop", 10, 6.0,
        { -1.6, 4.8, 7.2, 8, 5.6, -1.11022e-15, -2.4, -2.4, -1.6, -1.6 }
    },
    {
        "reggae", 10, 8.0,
        { -1.11022e-15, -1.11022e-15, -1.11022e-15, -5.6, -1.11022e-15, 6.4,
          6.4, -1.11022e-15, -1.11022e-15, -1.11022e-15 }
    },
    {
        "rock", 10, 5.0,
        { 8, 4.8, -5.6, -8, -3.2, 4, 8.8, 11.2, 11.2, 11.2 }
    },
    {
        "ska", 10, 6.0,
        { -2.4, -4.8, -4, -1.11022e-15, 4, 5.6, 8.8, 9.6, 11.2, 9.6 }
    },
    {
        "soft", 10, 5.0,
        { 4.8, 1.6, -1.11022e-15, -2.4, -1.11022e-15, 4, 8, 9.6, 11.2, 12 }
    },
    {
        "softrock", 10, 7.0,
        { 4, 4, 2.4, -1.11022e-15, -4, -5.6, -3.2, -1.11022e-15, 2.4, 8.8 }
    },
    {
        "techno", 10, 5.0,
        { 8, 5.6, -1.11022e-15, -5.6, -4.8, -1.11022e-15, 8, 9.6, 9.6, 8.8 }
    },
};


/*****************************************************************************
 * libvlc_audio_equalizer_get_preset_count : Get the number of equalizer presets
 *****************************************************************************/
unsigned libvlc_audio_equalizer_get_preset_count(void)
{
    return NB_PRESETS;
}

/*****************************************************************************
 * libvlc_audio_equalizer_get_preset_name : Get the name for a preset
 *****************************************************************************/
const char *libvlc_audio_equalizer_get_preset_name (unsigned u_index)
{
    if (u_index < NB_PRESETS)
	    return preset_list_text[u_index];
	   else
		return "<>";
}

/*****************************************************************************
 * libvlc_audio_equalizer_get_band_count : Get the number of equalizer frequency bands
 *****************************************************************************/
unsigned libvlc_audio_equalizer_get_band_count (void)
{
    return EQZ_BANDS_MAX;
}

/*****************************************************************************
 * libvlc_audio_equalizer_get_band_frequency : Get the frequency for a band
 *****************************************************************************/
double libvlc_audio_equalizer_get_band_frequency (unsigned u_index)
{
    if (u_index >= EQZ_BANDS_MAX)
        return -1.0;
	else
    	return f_vlc_frequency_table_10b[u_index];
}

/*****************************************************************************
 * libvlc_audio_equalizer_new : Create a new audio equalizer with zeroed values
 *****************************************************************************/
libvlc_equalizer_t *libvlc_audio_equalizer_new (void)
{
	
    libvlc_equalizer_t *p_equalizer;

    p_equalizer = my_malloc(sizeof(*p_equalizer));
    if (unlikely(p_equalizer == NULL))
        return NULL;

    p_equalizer->f_preamp = 0.0;

    for (unsigned i = 0; i < EQZ_BANDS_MAX; i++)
        p_equalizer->f_amp[i] = 0.0;

    return p_equalizer;
}

/*****************************************************************************
 * libvlc_audio_equalizer_new_from_preset : Create a new audio equalizer based on a preset
 *****************************************************************************/
libvlc_equalizer_t *libvlc_audio_equalizer_new_from_preset (unsigned u_index)
{
	
    libvlc_equalizer_t *p_equalizer;

    if (u_index >= NB_PRESETS)
        return NULL;

    p_equalizer = my_malloc(sizeof(*p_equalizer));
    if (unlikely(p_equalizer == NULL))
        return NULL;

    p_equalizer->f_preamp = eqz_preset_10b[u_index].f_preamp;

    for (unsigned i = 0; i < EQZ_BANDS_MAX; i++)
        p_equalizer->f_amp[i] = eqz_preset_10b[u_index].f_amp[i];

    return p_equalizer;
}

/*****************************************************************************
 * libvlc_audio_equalizer_release : Release a previously created equalizer
 *****************************************************************************/
void libvlc_audio_equalizer_release (libvlc_equalizer_t *p_equalizer)
{
	if (p_equalizer)
    	my_free(p_equalizer);
}

/*****************************************************************************
 * libvlc_audio_equalizer_set_preamp : Set the preamp value for an equalizer
 *****************************************************************************/
int libvlc_audio_equalizer_set_preamp (libvlc_equalizer_t *p_equalizer, double f_preamp)
{
    if (!p_equalizer)
        return -1;

    if (f_preamp < -20.0)
        f_preamp = -20.0;
    else if (f_preamp > 20.0)
        f_preamp = 20.0;

    p_equalizer->f_preamp = f_preamp;
    return 0;
}

/*****************************************************************************
 * libvlc_audio_equalizer_get_preamp : Get the preamp value for an equalizer
 *****************************************************************************/
double libvlc_audio_equalizer_get_preamp (libvlc_equalizer_t *p_equalizer)
{
    if (!p_equalizer)
        return 0.0;
	else
    	return p_equalizer->f_preamp;
}

/*****************************************************************************
 * libvlc_audio_equalizer_set_amp_at_index : Set the amplification value for an equalizer band
 *****************************************************************************/
int libvlc_audio_equalizer_set_amp_at_index (libvlc_equalizer_t *p_equalizer, double f_amp, unsigned u_band)
{
    if (!p_equalizer || u_band >= EQZ_BANDS_MAX)
        return -1;

    if (f_amp < -20.0)
        f_amp = -20.0;
    else if (f_amp > 20.0)
        f_amp = 20.0;

    p_equalizer->f_amp[u_band] = f_amp;
    return 0;
}

/*****************************************************************************
 * libvlc_audio_equalizer_get_amp_at_index : Get the amplification value for an equalizer band
 *****************************************************************************/
double libvlc_audio_equalizer_get_amp_at_index (libvlc_equalizer_t *p_equalizer, unsigned u_band)
{
    if (!p_equalizer || u_band >= EQZ_BANDS_MAX)
        return 0.0;
	else
		return p_equalizer->f_amp[u_band];
}

#if (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 0)
static inline int apply_equalizer (input_thread_t *p_input, libvlc_media_player_t *p_mi)
{

	int ret = 0;
    audio_output_t *p_aout = (audio_output_t*)input_GetAout(p_input);
    if (p_aout){
        char *psz_bands = var_GetString((vlc_object_t*)p_mi, "equalizer-bands");
        //printf("apply_equalizer bands: %p\n", psz_bands);
        
        if (psz_bands){
        	if (*psz_bands){
            	var_SetString((vlc_object_t*)p_aout, "equalizer-bands", psz_bands);
            	var_SetFloat((vlc_object_t*)p_aout, "equalizer-preamp", var_GetFloat((vlc_object_t*)p_mi, "equalizer-preamp"));
            	aout_EnableFilter((vlc_object_t*)p_mi, "equalizer", true);
            	ret = 1;
            }
        	free(psz_bands);            
        }

        vlc_object_release((vlc_object_t*)p_aout);
    //}else{
    	//printf("apply_equalizer: no aout\n");
    }
    return ret;
}
#endif

int libvlc_media_player_set_equalizer (libvlc_media_player_t *p_mi, libvlc_equalizer_t *p_equalizer)
{
    if (!p_equalizer){
#if (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 0)
        aout_EnableFilter((vlc_object_t*)p_mi, "equalizer", false);
#endif
        return 0;
    }

    char psz_band[16] = {0};
    char psz_bands[(EQZ_BANDS_MAX*16)+1] = {0};
    
    for (int i = 0; i < EQZ_BANDS_MAX; i++){
    	__mingw_snprintf(psz_band, sizeof(psz_band)-1, "%.07f ", p_equalizer->f_amp[i]);
    	strncat(psz_bands, psz_band, sizeof(psz_band)-1);
		psz_bands[EQZ_BANDS_MAX*16] = 0;
    }
    
    input_thread_t *p_input_thread = libvlc_get_input_thread(p_mi);
    if (!p_input_thread) return 0;
        
    int ret = 0;
	input_thread_private_t *p = p_input_thread->p;
	if (p){
		vlc_mutex_lock(&p->lock_control);
		vlc_mutex_lock(&p->p_item->lock);

#if (LIBVLC_VERSION_MAJOR == 2 && LIBVLC_VERSION_MINOR == 0)

		char *str = var_GetString((vlc_object_t*)p_mi, "equalizer-bands");
		if (!str){
			var_Create((vlc_object_t*)p_mi, "equalizer-preamp", VLC_VAR_FLOAT | VLC_VAR_DOINHERIT);
			var_Create((vlc_object_t*)p_mi, "equalizer-bands", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
			aout_EnableFilter((vlc_object_t*)p_mi, "equalizer", true);
		}else{
			free(str);
		}

    	var_SetFloat((vlc_object_t*)p_mi, "equalizer-preamp", p_equalizer->f_preamp);
    	var_SetString((vlc_object_t*)p_mi, "equalizer-bands", psz_bands);
    	
    	ret = apply_equalizer(p_input_thread, p_mi);
#else
/*
		var_SetFloat( p_mi, "equalizer-preamp", f_preamp );
		var_SetString( p_mi, "equalizer-bands", psz_bands );

		audio_output_t *p_aout = input_resource_HoldAout( p_mi->input.p_resource );
		if ( p_aout ){
			var_SetFloat( p_aout, "equalizer-preamp", f_preamp );
			var_SetString( p_aout, "equalizer-bands", psz_bands );

			vlc_object_release( p_aout );
		}
*/
#endif
    	vlc_mutex_unlock(&p->p_item->lock);
		vlc_mutex_unlock(&p->lock_control);
    }
    
    vlc_object_release(p_input_thread);
    return ret;
}
#endif
