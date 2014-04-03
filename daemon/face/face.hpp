/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_FACE_HPP
#define NFD_FACE_FACE_HPP

#include "common.hpp"
#include "core/event-emitter.hpp"
#include "core/face-uri.hpp"
#include "face-counter.hpp"

namespace nfd {

/** \class FaceId
 *  \brief identifies a face
 */
typedef int FaceId;

const FaceId INVALID_FACEID = -1;

const size_t MAX_NDN_PACKET_SIZE = 8800;


/** \brief represents a face
 */
class Face : noncopyable, public enable_shared_from_this<Face>
{
public:
  /**
   * \brief Face-related error
   */
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  Face(const FaceUri& remoteUri, const FaceUri& localUri, bool isLocal = false);

  virtual
  ~Face();

  /// fires when an Interest is received
  EventEmitter<Interest> onReceiveInterest;

  /// fires when a Data is received
  EventEmitter<Data> onReceiveData;

  /// fires when an Interest is sent out
  EventEmitter<Interest> onSendInterest;

  /// fires when a Data is sent out
  EventEmitter<Data> onSendData;

  /// fires when face disconnects or fails to perform properly
  EventEmitter<std::string/*reason*/> onFail;

  /// send an Interest
  virtual void
  sendInterest(const Interest& interest) = 0;

  /// send a Data
  virtual void
  sendData(const Data& data) = 0;

  /** \brief Close the face
   *
   *  This terminates all communication on the face and cause
   *  onFail() method event to be invoked
   */
  virtual void
  close() = 0;

public: // attributes
  FaceId
  getId() const;

  /** \brief Set the description
   *
   *  This is typically invoked by mgmt on set description command
   */
  virtual void
  setDescription(const std::string& description);

  /// Get the description
  virtual const std::string&
  getDescription() const;

  /** \brief Get whether face is connected to a local app
   */
  bool
  isLocal() const;

  /** \brief Get whether packets sent this Face may reach multiple peers
   *
   *  In this base class this property is always false.
   */
  virtual bool
  isMultiAccess() const;

  /** \brief Get whether underlying communication is up
   *
   *  In this base class this property is always true.
   */
  virtual bool
  isUp() const;

  /** \brief Get whether face is created on demand or explicitly via FaceManagement protocol
   */
  bool
  isOnDemand() const;

  const FaceCounters&
  getCounters() const;

  /** \deprecated use getRemoteUri instead
   */
  const FaceUri&
  getUri() const;

  /** \return a FaceUri that represents the remote endpoint
   */
  const FaceUri&
  getRemoteUri() const;

  /** \return a FaceUri that represents the local endpoint (NFD side)
   */
  const FaceUri&
  getLocalUri() const;

protected:
  // this is a non-virtual method
  bool
  decodeAndDispatchInput(const Block& element);

  FaceCounters&
  getMutableCounters();

  void
  setOnDemand(bool isOnDemand);

private:
  void
  setId(FaceId faceId);

private:
  FaceId m_id;
  std::string m_description;
  bool m_isLocal; // for scoping purposes
  FaceCounters m_counters;
  FaceUri m_remoteUri;
  FaceUri m_localUri;
  bool m_isOnDemand;

  // allow setting FaceId
  friend class FaceTable;
};


inline bool
Face::isLocal() const
{
  return m_isLocal;
}

inline const FaceCounters&
Face::getCounters() const
{
  return m_counters;
}

inline FaceCounters&
Face::getMutableCounters()
{
  return m_counters;
}

inline const FaceUri&
Face::getUri() const
{
  return this->getRemoteUri();
}

inline const FaceUri&
Face::getRemoteUri() const
{
  return m_remoteUri;
}

inline const FaceUri&
Face::getLocalUri() const
{
  return m_localUri;
}

inline void
Face::setOnDemand(bool isOnDemand)
{
  m_isOnDemand = isOnDemand;
}

inline bool
Face::isOnDemand() const
{
  return m_isOnDemand;
}

} // namespace nfd

#endif // NFD_FACE_FACE_HPP
