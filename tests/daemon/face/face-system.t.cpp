/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "face/face-system.hpp"
#include "face-system-fixture.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestFaceSystem, FaceSystemFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

class DummyProtocolFactory : public ProtocolFactory
{
public:
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override
  {
    processConfigHistory.push_back({configSection, context.isDryRun});
    if (!context.isDryRun) {
      this->providedSchemes = this->newProvidedSchemes;
    }
  }

  void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             bool wantLocalFieldsEnabled,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override
  {
    BOOST_FAIL("createFace should not be called");
  }

  std::vector<shared_ptr<const Channel>>
  getChannels() const override
  {
    BOOST_FAIL("getChannels should not be called");
    return {};
  }

public:
  struct ProcessConfigArgs
  {
    OptionalConfigSection configSection;
    bool isDryRun;
  };
  std::vector<ProcessConfigArgs> processConfigHistory;

  std::set<std::string> newProvidedSchemes;
};

BOOST_AUTO_TEST_CASE(Normal)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  auto f2 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;
  faceSystem.m_factories["f2"] = f2;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
        key v1
      }
      f2
      {
        key v2
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");
}

BOOST_AUTO_TEST_CASE(OmittedSection)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  auto f2 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;
  faceSystem.m_factories["f2"] = f2;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, true));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, true);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);

  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, false);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);
}

BOOST_AUTO_TEST_CASE(UnknownSection)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f0
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ChangeProvidedSchemes)
{
  auto f1 = make_shared<DummyProtocolFactory>();
  faceSystem.m_factories["f1"] = f1;

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  f1->newProvidedSchemes.insert("s1");
  f1->newProvidedSchemes.insert("s2");
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1.get());
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s2"), f1.get());

  f1->newProvidedSchemes.erase("s2");
  f1->newProvidedSchemes.insert("s3");
  BOOST_CHECK_NO_THROW(parseConfig(CONFIG, false));
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1.get());
  BOOST_CHECK(faceSystem.getFactoryByScheme("s2") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s3"), f1.get());
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_SUITE_END() // TestFaceSystem
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace face
} // namespace nfd
