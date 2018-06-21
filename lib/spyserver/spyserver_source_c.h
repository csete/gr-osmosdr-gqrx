/* -*- c++ -*- */
/*
 * Copyright 2013 Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_SPYSERVER_SOURCE_C_H
#define INCLUDED_SPYSERVER_SOURCE_C_H

#include <thread>
#include <atomic>
#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <gnuradio/sync_block.h>

#include "source_iface.h"
#include "spyserver_protocol.h"
#include "tcp_client.h"

class spyserver_source_c;

/*
 * We use boost::shared_ptr's instead of raw pointers for all access
 * to gr::blocks (and many other data structures).  The shared_ptr gets
 * us transparent reference counting, which greatly simplifies storage
 * management issues.  This is especially helpful in our hybrid
 * C++ / Python system.
 *
 * See http://www.boost.org/libs/smart_ptr/smart_ptr.htm
 *
 * As a convention, the _sptr suffix indicates a boost::shared_ptr
 */
typedef boost::shared_ptr<spyserver_source_c> spyserver_source_c_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of spyserver_source_c.
 *
 * To avoid accidental use of raw pointers, spyserver_source_c's
 * constructor is private.  make_spyserver_source_c is the public
 * interface for creating new instances.
 */
spyserver_source_c_sptr make_spyserver_source_c (const std::string & args = "");

/*!
 * \brief Provides a stream of complex samples.
 * \ingroup block
 */
class spyserver_source_c :
    public gr::sync_block,
    public source_iface
{
private:
  // The friend declaration allows make_spyserver_source_c to
  // access the private constructor.

  friend spyserver_source_c_sptr make_spyserver_source_c (const std::string & args);

  /*!
   * \brief Provides a stream of complex samples.
   */
  spyserver_source_c (const std::string & args);   // private constructor


public:
  ~spyserver_source_c ();  // public destructor

  bool start();
  bool stop();

  int work( int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items );

  static std::vector< std::string > get_devices(bool fake = false);

  size_t get_num_channels( void );

  osmosdr::meta_range_t get_sample_rates( void );
  double set_sample_rate( double rate );
  double get_sample_rate( void );

  osmosdr::freq_range_t get_freq_range( size_t chan = 0 );
  double set_center_freq( double freq, size_t chan = 0 );
  double get_center_freq( size_t chan = 0 );
  double set_freq_corr( double ppm, size_t chan = 0 );
  double get_freq_corr( size_t chan = 0 );

  std::vector<std::string> get_gain_names( size_t chan = 0 );
  osmosdr::gain_range_t get_gain_range( size_t chan = 0 );
  osmosdr::gain_range_t get_gain_range( const std::string & name, size_t chan = 0 );
  bool set_gain_mode( bool automatic, size_t chan = 0 );
  bool get_gain_mode( size_t chan = 0 );
  double set_gain( double gain, size_t chan = 0 );
  double set_gain( double gain, const std::string & name, size_t chan = 0 );
  double get_gain( size_t chan = 0 );
  double get_gain( const std::string & name, size_t chan = 0 );

  double set_lna_gain( double gain, size_t chan = 0 );
  double set_mix_gain(double gain, size_t chan = 0 );
  double set_if_gain( double gain, size_t chan = 0 );
  double set_bb_gain( double gain, size_t chan = 0 ) { return set_mix_gain(gain, chan); };

  std::vector< std::string > get_antennas( size_t chan = 0 );
  std::string set_antenna( const std::string & antenna, size_t chan = 0 );
  std::string get_antenna( size_t chan = 0 );

  double set_bandwidth( double bandwidth, size_t chan = 0 );
  double get_bandwidth( size_t chan = 0 );
  osmosdr::freq_range_t get_bandwidth_range( size_t chan = 0 );

  void set_biast( bool enabled );
  bool get_biast();

private:
  static constexpr unsigned int BufferSize = 64 * 1024;
  const uint32_t ProtocolVersion = SPYSERVER_PROTOCOL_VERSION;
  const std::string SoftwareID = std::string("gr-osmosdr");
  const std::string NameNoDevice = std::string("SpyServer - No Device");
  const std::string NameAirspyOne = std::string("SpyServer - Airspy One");
  const std::string NameAirspyHF = std::string("SpyServer - Airspy HF+");
  const std::string NameRTLSDR = std::string("SpyServer - RTLSDR");
  const std::string NameUnknown = std::string("SpyServer - Unknown Device");

  uint32_t minimum_tunable_frequency;
  uint32_t maximum_tunable_frequency;
  uint32_t device_center_frequency;
  uint32_t channel_center_frequency;
  uint32_t channel_decimation_stage_count;
  int32_t gain;
  tcp_client client;

  void connect();
  void disconnect();
  void thread_loop();
  bool say_hello();
  void cleanup();
  void on_connect();

  bool set_setting(uint32_t settingType, std::vector<uint32_t> params);
  bool send_command(uint32_t cmd, std::vector<uint8_t> args);
  void parse_message(char *buffer, uint32_t len);
  int parse_header(char *buffer, uint32_t len);
  int parse_body(char *buffer, uint32_t len);
  void process_device_info();
  void process_client_sync();
  void process_uint8_samples();
  void process_int16_samples();
  void process_float_samples();
  void process_uint8_fft();
  void handle_new_message();
  void set_stream_state();

  std::atomic_bool terminated;
  std::atomic_bool streaming;
  std::atomic_bool got_device_info;
  std::atomic_bool got_sync_info;
  std::atomic_bool can_control;
  std::atomic_bool is_connected;
  std::thread *receiver_thread;

  uint32_t dropped_buffers;
  std::atomic<int64_t> down_stream_bytes;

  uint8_t *header_data;
  uint8_t *body_buffer;
  uint64_t body_buffer_length;
  uint32_t parser_position;
  uint32_t last_sequence_number;

  std::string ip;
  int port;

  DeviceInfo device_info;
  MessageHeader header;

  uint32_t streaming_mode;
  uint32_t parser_phase;

  boost::circular_buffer<gr_complex> *_fifo;
  boost::mutex _fifo_lock;
  boost::condition_variable _samp_avail;

  std::vector< std::pair<double, uint32_t> > _sample_rates;
  double _sample_rate;
  double _center_freq;
  double _gain;
};

#endif /* INCLUDED_SPYSERVER_SOURCE_C_H */
