#ifndef _ZNP_API_H_
#define _ZNP_API_H_
#include <bitset>
#include <boost/signals2/signal.hpp>
#include <map>
#include <queue>
#include <set>
#include <stlab/concurrency/future.hpp>
#include <vector>
#include "logging.h"
#include "polyfill/apply.h"
#include "znp/encoding.h"
#include "znp/znp.h"
#include "znp/znp_raw_interface.h"

namespace znp {
class ZnpApi {
 public:
  ZnpApi(std::shared_ptr<ZnpRawInterface> interface);
  ~ZnpApi() = default;

  // SYS commands
  stlab::future<ResetInfo> SysReset(bool soft_reset);
  stlab::future<Capability> SysPing();
  stlab::future<void> SysOsalNvItemInitRaw(NvItemId Id, uint16_t ItemLen,
                                           std::vector<uint8_t> InitData);
  stlab::future<std::vector<uint8_t>> SysOsalNvReadRaw(NvItemId Id,
                                                       uint8_t Offset);
  stlab::future<void> SysOsalNvWriteRaw(NvItemId Id, uint8_t Offset,
                                        std::vector<uint8_t> Value);
  stlab::future<void> SysOsalNvDelete(NvItemId Id, uint16_t ItemLen);
  stlab::future<uint16_t> SysOsalNvLength(NvItemId Id);

  // SYS events
  boost::signals2::signal<void(ResetInfo)> sys_on_reset_;

  // AF commands
  stlab::future<void> AfRegister(uint8_t endpoint, uint16_t profile_id,
                                 uint16_t device_id, uint8_t version,
                                 Latency latency,
                                 std::vector<uint16_t> input_clusters,
                                 std::vector<uint16_t> output_clusters);
  stlab::future<void> AfDataRequest(ShortAddress DstAddr, uint8_t DstEndpoint,
                                    uint8_t SrcEndpoint, uint16_t ClusterId,
                                    uint8_t TransId, uint8_t Options,
                                    uint8_t Radius, std::vector<uint8_t> Data);
  // AF events
  boost::signals2::signal<void(const IncomingMsg&)> af_on_incoming_msg_;

  // ZDO commands
  stlab::future<ZdoIEEEAddressResponse> ZdoIEEEAddress(
      ShortAddress address, boost::optional<uint8_t> children_index);
  stlab::future<void> ZdoRemoveLinkKey(IEEEAddress IEEEAddr);
  stlab::future<std::tuple<IEEEAddress, std::array<uint8_t, 16>>> ZdoGetLinkKey(
      IEEEAddress IEEEAddr);
  stlab::future<ShortAddress> ZdoMgmtLeave(ShortAddress DstAddr,
                                           IEEEAddress DeviceAddr,
                                           uint8_t remove_rejoin);
  stlab::future<uint16_t> ZdoMgmtPermitJoin(AddrMode addr_mode,
                                            uint16_t dst_address,
                                            uint8_t duration,
                                            uint8_t tc_significance);
  stlab::future<StartupFromAppResponse> ZdoStartupFromApp(
      uint16_t start_delay_ms);

  // ZDO events
  boost::signals2::signal<void(DeviceState)> zdo_on_state_change_;
  boost::signals2::signal<void(uint8_t)> zdo_on_permit_join_;

  // SAPI commands
  stlab::future<std::vector<uint8_t>> SapiReadConfigurationRaw(
      ConfigurationOption option);
  template <ConfigurationOption O>
  stlab::future<typename ConfigurationOptionInfo<O>::Type>
  SapiReadConfiguration() {
    return SapiReadConfigurationRaw(O).then(
        &znp::Decode<typename ConfigurationOptionInfo<O>::Type>);
  }
  stlab::future<void> SapiWriteConfigurationRaw(
      ConfigurationOption option, const std::vector<uint8_t>& value);
  template <ConfigurationOption O>
  stlab::future<void> SapiWriteConfiguration(
      const typename ConfigurationOptionInfo<O>::Type& value) {
    return SapiWriteConfigurationRaw(O, znp::Encode(value));
  }

  stlab::future<std::vector<uint8_t>> SapiGetDeviceInfoRaw(DeviceInfo info);
  template <DeviceInfo I>
  stlab::future<typename DeviceInfoInfo<I>::Type> SapiGetDeviceInfo() {
    // DecodePartial because GetDeviceInfo will always return 8 bytes, even if
    // less are needed.
    return SapiGetDeviceInfoRaw(I).then(
        &znp::DecodePartial<typename DeviceInfoInfo<I>::Type>);
  }

  // SAPI events

  // UTIL commands
  stlab::future<IEEEAddress> UtilAddrmgrNwkAddrLookup(ShortAddress address);
  // UTIL events

  // Helper functions
  stlab::future<DeviceState> WaitForState(std::set<DeviceState> end_states,
                                          std::set<DeviceState> allowed_states);

 private:
  std::shared_ptr<ZnpRawInterface> raw_;
  boost::signals2::scoped_connection on_frame_connection_;

  struct FrameHandlerAction {
    bool
        stop_processing;  // If true, do not call handlers further down the list
    bool remove_me;  // If true, remove this handler from the list, and do not
                     // call again.
  };
  typedef std::function<FrameHandlerAction(
      const ZnpCommandType&, const ZnpCommand&, const std::vector<uint8_t>&)>
      FrameHandler;
  std::list<FrameHandler> handlers_;

  void OnFrame(ZnpCommandType type, ZnpCommand command,
               const std::vector<uint8_t>& payload);
  stlab::future<std::vector<uint8_t>> WaitFor(ZnpCommandType type,
                                              ZnpCommand command);
  stlab::future<std::vector<uint8_t>> WaitAfter(
      stlab::future<void> first_request, ZnpCommandType type,
      ZnpCommand command);
  stlab::future<std::vector<uint8_t>> RawSReq(
      ZnpCommand command, const std::vector<uint8_t>& payload);
  static std::vector<uint8_t> CheckStatus(const std::vector<uint8_t>& response);
  static void CheckOnlyStatus(const std::vector<uint8_t>& response);

  template <typename... Args>
  void AddSimpleEventHandler(ZnpCommandType type, ZnpCommand command,
                             boost::signals2::signal<void(Args...)>& signal,
                             bool allow_partial) {
    handlers_.push_back([&signal, type, command, allow_partial](
                            const ZnpCommandType& recvd_type,
                            const ZnpCommand& recvd_command,
                            const std::vector<uint8_t>& data)
                            -> FrameHandlerAction {
      if (recvd_type != type || recvd_command != command) {
        return {false, false};
      }
      typedef std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>
          ArgTuple;
      ArgTuple arguments;
      try {
        if (allow_partial) {
          arguments = znp::DecodePartial<ArgTuple>(data);
        } else {
          arguments = znp::Decode<ArgTuple>(data);
        }
      } catch (const std::exception& exc) {
        LOG("ZnpApi", warning)
            << "Exception while decoding event: " << exc.what();
        return {false, false};
      }
      polyfill::apply(signal, arguments);
      return {true, false};
    });
  }
};
}  // namespace znp
#endif  //_ZNP_API_H_
