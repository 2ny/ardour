/*
    Copyright (C) 2016 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __ardour_push2_h__
#define __ardour_push2_h__

#include <vector>
#include <map>
#include <list>
#include <set>

#include <libusb.h>

#include <cairomm/refptr.h>
#include <glibmm/threads.h>

#define ABSTRACT_UI_EXPORTS
#include "pbd/abstract_ui.h"
#include "midi++/types.h"
#include "ardour/types.h"
#include "control_protocol/control_protocol.h"

#include "midi_byte_array.h"

namespace Cairo {
	class ImageSurface;
}

namespace MIDI {
	class Parser;
	class Port;
}

namespace ARDOUR {
	class AsyncMIDIPort;
	class Port;
}

namespace ArdourSurface {

struct Push2Request : public BaseUI::BaseRequestObject {
public:
	Push2Request () {}
	~Push2Request () {}
};

class Push2 : public ARDOUR::ControlProtocol
            , public AbstractUI<Push2Request>
{
   public:
	Push2 (ARDOUR::Session&);
	~Push2 ();

	static bool probe ();
	static void* request_factory (uint32_t);

	int set_active (bool yn);
	XMLNode& get_state();
	int set_state (const XMLNode & node, int version);

   private:
	libusb_device_handle *handle;
	Glib::Threads::Mutex fb_lock;
	uint8_t   frame_header[16];
	uint16_t* device_frame_buffer[2];
	int  device_buffer;
	Cairo::RefPtr<Cairo::ImageSurface> frame_buffer;
	sigc::connection vblank_connection;
	sigc::connection periodic_connection;

	static const int cols;
	static const int rows;
	static const int pixels_per_row;

	void do_request (Push2Request*);
	int stop ();
	int open ();
	int close ();
	int render ();
	bool vblank ();

	enum ButtonID {
		TapTempo,
		Metronome,
		Upper1, Upper2, Upper3, Upper4, Upper5, Upper6, Upper7, Upper8,
		Setup,
		User,
		Delete,
		AddDevice,
		Device,
		Mix,
		Undo,
		AddTrack,
		Browse,
		Clip,
		Mute,
		Solo,
		Stop,
		Lower1, Lower2, Lower3, Lower4, Lower5, Lower6, Lower7, Lower8,
		Master,
		Convert,
		DoubleLoop,
		Quantize,
		Duplicate,
		New,
		FixedLength,
		Automate,
		RecordEnable,
		Play,
		Fwd32ndT,
		Fwd32nd,
		Fwd16thT,
		Fwd16th,
		Fwd8thT,
		Fwd8th,
		Fwd4trT,
		Fwd4tr,
		Up,
		Right,
		Down,
		Left,
		Repeat,
		Accent,
		Scale,
		Layout,
		Note,
		Session,
		OctaveUp,
		PageRight,
		OctaveDown,
		PageLeft,
		Shift,
		Select
	};

	struct LED
	{
		enum State {
			Off,
			OneShot24th,
			OneShot16th,
			OneShot8th,
			OneShot4th,
			OneShot2th,
			Pulsing24th,
			Pulsing16th,
			Pulsing8th,
			Pulsing4th,
			Pulsing2th,
			Blinking24th,
			Blinking16th,
			Blinking8th,
			Blinking4th,
			Blinking2th
		};

		LED (uint8_t e) : _extra (e), _color_index (0), _state (Off) {}
		virtual ~LED() {}

		uint8_t extra () const { return _extra; }
		uint8_t color_index () const { return _color_index; }
		State   state () const { return _state; }

		void set_color (uint8_t color_index);
		void set_state (State state);

		virtual MidiByteArray state_msg() const = 0;

	     protected:
		uint8_t _extra;
		uint8_t _color_index;
		State   _state;
	};

	struct Pad : public LED {
		Pad (int xx, int yy, uint8_t ex)
			: LED (ex)
			, x (xx)
			, y (yy) {}

		MidiByteArray state_msg () const { return MidiByteArray (3, 0x90|_state, _extra, (_state == Off) ? 0 : _color_index); }

		int coord () const { return (y * 8) + x; }
		int note_number() const { return extra(); }

		int x;
		int y;
	};

	struct Button : public LED {
		Button (ButtonID bb, uint8_t ex)
			: LED (ex)
			, id (bb)
			, press_method (&Push2::relax)
			, release_method (&Push2::relax)
		{}

		Button (ButtonID bb, uint8_t ex, void (Push2::*press)())
			: LED (ex)
			, id (bb)
			, press_method (press)
			, release_method (&Push2::relax)
		{}

		Button (ButtonID bb, uint8_t ex, void (Push2::*press)(), void (Push2::*release)())
			: LED (ex)
			, id (bb)
			, press_method (press)
			, release_method (release)
		{}

		MidiByteArray state_msg () const { return MidiByteArray (3, 0xb0|_state, _extra, (_state == Off) ? 0 : _color_index); }
		int controller_number() const { return extra(); }

		ButtonID id;
		void (Push2::*press_method)();
		void (Push2::*release_method)();
	};

	struct ColorButton : public Button {
		ColorButton (ButtonID bb, uint8_t ex)
			: Button (bb, ex) {}


		ColorButton (ButtonID bb, uint8_t ex, void (Push2::*press)())
			: Button (bb, ex, press) {}

		ColorButton (ButtonID bb, uint8_t ex, void (Push2::*press)(), void (Push2::*release)())
			: Button (bb, ex, press, release) {}
	};

	struct WhiteButton : public Button {
		WhiteButton (ButtonID bb, uint8_t ex)
			: Button (bb, ex) {}

		WhiteButton (ButtonID bb, uint8_t ex, void (Push2::*press)())
			: Button (bb, ex, press) {}

		WhiteButton (ButtonID bb, uint8_t ex, void (Push2::*press)(), void (Push2::*release)())
			: Button (bb, ex, press, release) {}
	};

	void relax () {}

	/* map of Buttons by CC */
	typedef std::map<int,Button*> CCButtonMap;
	CCButtonMap cc_button_map;
	/* map of Buttons by ButtonID */
	typedef std::map<ButtonID,Button*> IDButtonMap;
	IDButtonMap id_button_map;

	/* map of Pads by note number */
	typedef std::map<int,Pad*> NNPadMap;
	NNPadMap nn_pad_map;
	/* map of Pads by coordinate
	 *
	 * coord = row * 64 + column;
	 *
	 * rows start at top left
	 */
	typedef std::map<int,Pad*> CoordPadMap;
	CoordPadMap coord_pad_map;

	void set_button_color (ButtonID, uint8_t color_index);
	void set_button_state (ButtonID, LED::State);
	void set_led_color (ButtonID, uint8_t color_index);
	void set_led_state (ButtonID, LED::State);

	void build_maps ();

	MIDI::Port* _input_port[2];
	MIDI::Port* _output_port[2];
	boost::shared_ptr<ARDOUR::Port> _async_in[2];
	boost::shared_ptr<ARDOUR::Port> _async_out[2];

	void connect_to_parser ();
	void handle_midi_pitchbend_message (MIDI::Parser&, MIDI::pitchbend_t);
	void handle_midi_controller_message (MIDI::Parser&, MIDI::EventTwoBytes*);
	void handle_midi_note_on_message (MIDI::Parser&, MIDI::EventTwoBytes*);
	void handle_midi_note_off_message (MIDI::Parser&, MIDI::EventTwoBytes*);
	void handle_midi_sysex (MIDI::Parser&, MIDI::byte *, size_t count);

	void write (int port, const MidiByteArray&);
	bool midi_input_handler (Glib::IOCondition ioc, MIDI::Port* port);
	bool periodic ();

	void thread_init ();

	PBD::ScopedConnectionList session_connections;
	void connect_session_signals ();
	void notify_record_state_changed ();
	void notify_transport_state_changed ();
	void notify_loop_state_changed ();
	void notify_parameter_changed (std::string);
	void notify_solo_active_changed (bool);

	/* Button methods */
	void button_play ();
	void button_recenable ();
	void button_up ();
	void button_down ();
};


} /* namespace */

#endif /* __ardour_push2_h__ */