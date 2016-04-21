/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef TCPCONGESTIONFIXED_H
#define TCPCONGESTIONFIXED_H

#include "ns3/object.h"
#include "ns3/timer.h"
#include "ns3/tcp-congestion-ops.h"

namespace ns3 {

class TcpSocketState;
class TcpSocketBase;

/**
 * \brief Congestion control abstract class
 *
 * The design is inspired on what Linux v4.0 does (but it has been
 * in place since years). The congestion control is splitted from the main
 * socket code, and it is a pluggable component. An interface has been defined;
 * variables are maintained in the TcpSocketState class, while subclasses of
 * TcpCongestionOps operate over an instance of that class.
 *
 * Only three methods has been utilized right now; however, Linux has many others,
 * which can be added later in ns-3.
 *
 * \see IncreaseWindow
 * \see PktsAcked
 */
class TcpCongestionOps;

/**
 * \brief The Fixed implementation
 *
 * New Reno introduces partial ACKs inside the well-established Reno algorithm.
 * This and other modifications are described in RFC 6582.
 *
 * \see IncreaseWindow
 */
class TcpFixed : public TcpCongestionOps
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpFixed ();
  TcpFixed (const TcpFixed& sock);

  ~TcpFixed ();

  std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);
  virtual Ptr<TcpCongestionOps> Fork ();
};

} // namespace ns3

#endif // TCPCONGESTIONFIXED_H

