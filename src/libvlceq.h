
#ifndef _LIBVLCEQ_H_
#define _LIBVLCEQ_H_



#if (USEINTERNALEQ)

#define EQZ_BANDS_MAX	10		// libvlc_audio_equalizer_get_band_count
#define NB_PRESETS		18		// libvlc_audio_equalizer_get_preset_count



#if (LIBVLC_VERSION_MAJOR >= 2)

#if (LIBVLC_VERSION_MINOR == 0)
#include <vlc_aout_intf.h>
#endif

#include <vlc_access.h>

#else

#ifndef LIBVLC_API
#define LIBVLC_API
#endif

typedef struct audio_output audio_output_t;
#include <vlc_aout.h>

#endif 


 /**
 * Opaque equalizer handle.
 *
 * Equalizer settings can be applied to a media player.
 */
typedef struct libvlc_equalizer_t libvlc_equalizer_t;
 
/**
 * Get the number of equalizer presets.
 *
 * \return number of presets
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API unsigned libvlc_audio_equalizer_get_preset_count( void );

/**
 * Get the name of a particular equalizer preset.
 *
 * This name can be used, for example, to prepare a preset label or menu in a user
 * interface.
 *
 * \param u_index index of the preset, counting from zero
 * \return preset name, or NULL if there is no such preset
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API const char *libvlc_audio_equalizer_get_preset_name( unsigned u_index );

/**
 * Get the number of distinct frequency bands for an equalizer.
 *
 * \return number of frequency bands
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API unsigned libvlc_audio_equalizer_get_band_count( void );

/**
 * Get a particular equalizer band frequency.
 *
 * This value can be used, for example, to create a label for an equalizer band control
 * in a user interface.
 *
 * \param u_index index of the band, counting from zero
 * \return equalizer band frequency (Hz), or -1 if there is no such band
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API double libvlc_audio_equalizer_get_band_frequency( unsigned u_index );

/**
 * Create a new default equalizer, with all frequency values zeroed.
 *
 * The new equalizer can subsequently be applied to a media player by invoking
 * libvlc_media_player_set_equalizer().
 *
 * The returned handle should be freed via libvlc_audio_equalizer_release() when
 * it is no longer needed.
 *
 * \return opaque equalizer handle, or NULL on error
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API libvlc_equalizer_t *libvlc_audio_equalizer_new( void );

/**
 * Create a new equalizer, with initial frequency values copied from an existing
 * preset.
 *
 * The new equalizer can subsequently be applied to a media player by invoking
 * libvlc_media_player_set_equalizer().
 *
 * The returned handle should be freed via libvlc_audio_equalizer_release() when
 * it is no longer needed.
 *
 * \param u_index index of the preset, counting from zero
 * \return opaque equalizer handle, or NULL on error
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API libvlc_equalizer_t *libvlc_audio_equalizer_new_from_preset( unsigned u_index );

/**
 * Release a previously created equalizer instance.
 *
 * The equalizer was previously created by using libvlc_audio_equalizer_new() or
 * libvlc_audio_equalizer_new_from_preset().
 *
 * It is safe to invoke this method with a NULL p_equalizer parameter for no effect.
 *
 * \param p_equalizer opaque equalizer handle, or NULL
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API void libvlc_audio_equalizer_release( libvlc_equalizer_t *p_equalizer );

/**
 * Set a new pre-amplification value for an equalizer.
 *
 * The new equalizer settings are subsequently applied to a media player by invoking
 * libvlc_media_player_set_equalizer().
 *
 * \param p_equalizer opaque equalizer handle
 * \param f_preamp preamp value (-20.0 to 20.0 Hz)
 * \return zero on success, -1 on error
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API int libvlc_audio_equalizer_set_preamp( libvlc_equalizer_t *p_equalizer, double f_preamp );

/**
 * Get the current pre-amplification value from an equalizer.
 *
 * \param p_equalizer opaque equalizer handle
 * \return preamp value (Hz)
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API double libvlc_audio_equalizer_get_preamp( libvlc_equalizer_t *p_equalizer );

/**
 * Set a new amplification value for a particular equalizer frequency band.
 *
 * The new equalizer settings are subsequently applied to a media player by invoking
 * libvlc_media_player_set_equalizer().
 *
 * \param p_equalizer opaque equalizer handle
 * \param f_amp amplification value (-20.0 to 20.0 Hz)
 * \param u_band index, counting from zero, of the frequency band to set
 * \return zero on success, -1 on error
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API int libvlc_audio_equalizer_set_amp_at_index( libvlc_equalizer_t *p_equalizer, double f_amp, unsigned u_band );

/**
 * Get the amplification value for a particular equalizer frequency band.
 *
 * \param p_equalizer opaque equalizer handle
 * \param u_band index, counting from zero, of the frequency band to get
 * \return amplification value (Hz); zero if there is no such frequency band
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API double libvlc_audio_equalizer_get_amp_at_index( libvlc_equalizer_t *p_equalizer, unsigned u_band );

/**
 * Apply new equalizer settings to a media player.
 *
 * The equalizer is first created by invoking libvlc_audio_equalizer_new() or
 * libvlc_audio_equalizer_new_from_preset().
 *
 * It is possible to apply new equalizer settings to a media player whether the media
 * player is currently playing media or not.
 *
 * Invoking this method will immediately apply the new equalizer settings to the audio
 * output of the currently playing media if there is any.
 *
 * If there is no currently playing media, the new equalizer settings will be applied
 * later if and when new media is played.
 *
 * Equalizer settings will automatically be applied to subsequently played media.
 *
 * To disable the equalizer for a media player invoke this method passing NULL for the
 * p_equalizer parameter.
 *
 * The media player does not keep a reference to the supplied equalizer so it is safe
 * for an application to release the equalizer reference any time after this method
 * returns.
 *
 * \param p_mi opaque media player handle
 * \param p_equalizer opaque equalizer handle, or NULL to disable the equalizer for this media player
 * \return zero on success, -1 on error
 * \version LibVLC 2.1.0 or later
 */
LIBVLC_API int libvlc_media_player_set_equalizer( libvlc_media_player_t *p_mi, libvlc_equalizer_t *p_equalizer );


#endif

#endif
