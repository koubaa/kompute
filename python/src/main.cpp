#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>

#include <kompute/Kompute.hpp>

#include "docstrings.hpp"
#include "utils.hpp"

namespace py = pybind11;

// used in Core.hpp
py::object kp_trace, kp_debug, kp_info, kp_warning, kp_error;

static void kp_log(int level, const std::string& msg) {
  py::gil_scoped_acquire gil;
    switch (level) {
      case 0:
        kp_trace(msg); break;
      case 1:
        kp_debug(msg); break;
      case 2:
        kp_info(msg); break;
      case 3:
        kp_warning(msg); break;
      case 4:
        kp_error(msg); break;
      default:
        break;
    }
}

void py_log_trace(const std::string& msg) {kp_log(0, msg);}
void py_log_debug(const std::string& msg) {kp_log(1, msg);}
void py_log_info(const std::string& msg) {kp_log(2, msg);}
void py_log_warning(const std::string& msg) {kp_log(3, msg);}
void py_log_error(const std::string& msg) {kp_log(4, msg);}

std::unique_ptr<kp::OpAlgoDispatch>
opAlgoDispatchPyInit(std::shared_ptr<kp::Algorithm>& algorithm,
                     const py::array& push_consts)
{
    const py::buffer_info info = push_consts.request();
    KP_LOG_DEBUG("Kompute Python Manager creating tensor_T with push_consts "
                 "size {} dtype {}",
                 push_consts.size(),
                 std::string(py::str(push_consts.dtype())));

    if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
        std::vector<float> dataVec((float*)info.ptr,
                                   ((float*)info.ptr) + info.size);
        return std::unique_ptr<kp::OpAlgoDispatch>{ new kp::OpAlgoDispatch(
          algorithm, dataVec) };
    } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
        std::vector<uint32_t> dataVec((uint32_t*)info.ptr,
                                      ((uint32_t*)info.ptr) + info.size);
        return std::unique_ptr<kp::OpAlgoDispatch>{ new kp::OpAlgoDispatch(
          algorithm, dataVec) };
    } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
        std::vector<int32_t> dataVec((int32_t*)info.ptr,
                                     ((int32_t*)info.ptr) + info.size);
        return std::unique_ptr<kp::OpAlgoDispatch>{ new kp::OpAlgoDispatch(
          algorithm, dataVec) };
    } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
        std::vector<double> dataVec((double*)info.ptr,
                                    ((double*)info.ptr) + info.size);
        return std::unique_ptr<kp::OpAlgoDispatch>{ new kp::OpAlgoDispatch(
          algorithm, dataVec) };
    } else {
        throw std::runtime_error("Kompute Python no valid dtype supported");
    }
}

namespace pv {
  std::vector<uint32_t> to_vector(const py::bytes& spirv) {
    py::buffer_info info(py::buffer(spirv).request());
    const char* data = reinterpret_cast<const char*>(info.ptr);
    size_t length = static_cast<size_t>(info.size);
    std::vector<uint32_t> spirvVec((uint32_t*)data,
                                    (uint32_t*)(data + length));
    return spirvVec;
  }

  template<typename spec_T, typename push_T>
  std::shared_ptr<kp::Algorithm> algorithm(
    kp::Manager& mgr,
    const std::vector<std::shared_ptr<kp::Memory>>& tensors,
    const py::bytes& spirv,
    const kp::Workgroup& workgroup,
    const py::array& spec_consts,
    const py::array& push_consts) {
      std::vector<uint32_t> spirvVec = pv::to_vector(spirv);
      const py::buffer_info pushInfo = push_consts.request();
      const py::buffer_info specInfo = spec_consts.request();
      std::vector<spec_T> specConstsVec(
        (spec_T*)specInfo.ptr, ((spec_T*)specInfo.ptr) + specInfo.size);
      std::vector<push_T> pushConstsVec(
        (push_T*)pushInfo.ptr, ((push_T*)pushInfo.ptr) + pushInfo.size);
      return mgr.algorithm(
        tensors,
        spirvVec,
        workgroup,
        specConstsVec,
        pushConstsVec
      );
  }

  template<typename push_T>
  std::shared_ptr<kp::Algorithm> algorithm(
    kp::Manager& mgr,
    const std::vector<std::shared_ptr<kp::Memory>>& tensors,
    const py::bytes& spirv,
    const kp::Workgroup& workgroup,
    const std::vector<float>& specConsts,
    const py::array& push_consts) {
      std::vector<uint32_t> spirvVec = pv::to_vector(spirv);
      const py::buffer_info pushInfo = push_consts.request();
      std::vector<push_T> pushConstsVec(
        (push_T*)pushInfo.ptr, ((push_T*)pushInfo.ptr) + pushInfo.size);
      return mgr.algorithm(
        tensors,
        spirvVec,
        workgroup,
        specConsts,
        pushConstsVec
      );
  }

  template<typename spec_T>
  std::shared_ptr<kp::Algorithm> algorithm(
    kp::Manager& mgr,
    const std::vector<std::shared_ptr<kp::Memory>>& tensors,
    const py::bytes& spirv,
    const kp::Workgroup& workgroup,
    const py::array& specConsts,
    const std::vector<float>& pushConsts) {
      std::vector<uint32_t> spirvVec = pv::to_vector(spirv);
      const py::buffer_info specInfo = specConsts.request();
      std::vector<spec_T> specConstsVec(
        (spec_T*)specInfo.ptr, ((spec_T*)specInfo.ptr) + specInfo.size);
      return mgr.algorithm(
        tensors,
        spirvVec,
        workgroup,
        specConstsVec,
        pushConsts
      );
  }
}

PYBIND11_MODULE(kp, m)
{

    // The logging modules are used in the Kompute.hpp file
    py::module_ logging = py::module_::import("logging");
    py::object kp_logger = logging.attr("getLogger")("kp");
    kp_trace = kp_logger.attr(
      "debug"); // Same as for debug since python has no trace logging level
    kp_debug = kp_logger.attr("debug");
    kp_info = kp_logger.attr("info");
    kp_warning = kp_logger.attr("warning");
    kp_error = kp_logger.attr("error");
    logging.attr("basicConfig")();

    py::module_ np = py::module_::import("numpy");

    py::class_<vk::PhysicalDeviceFeatures>(
      m, "PhysicalDeviceFeatures")
      .def(py::init())
      .def_readwrite("robust_buffer_access", &vk::PhysicalDeviceFeatures::robustBufferAccess)
      .def_readwrite("shader_float64", &vk::PhysicalDeviceFeatures::shaderFloat64)
      .def_readwrite("shader_int64", &vk::PhysicalDeviceFeatures::shaderInt64)
      .def_readwrite("shader_int16", &vk::PhysicalDeviceFeatures::shaderInt16);

    py::enum_<kp::Memory::DataTypes>(m, "DataTypes")
      .value("bool",
             kp::Memory::DataTypes::eBool,
             DOC(kp, Memory, DataTypes, eBool))
      .value("int",
             kp::Memory::DataTypes::eInt,
             DOC(kp, Memory, DataTypes, eInt))
      .value("uint",
             kp::Memory::DataTypes::eUnsignedInt,
             DOC(kp, Memory, DataTypes, eUnsignedInt))
      .value("float",
             kp::Memory::DataTypes::eFloat,
             DOC(kp, Memory, DataTypes, eFloat))
      .value("double",
             kp::Memory::DataTypes::eDouble,
             DOC(kp, Memory, DataTypes, eDouble))
      .value("custom",
             kp::Memory::DataTypes::eCustom,
             DOC(kp, Memory, DataTypes, eCustom))
      .value("short",
             kp::Memory::DataTypes::eShort,
             DOC(kp, Memory, DataTypes, eShort))
      .value("ushort",
             kp::Memory::DataTypes::eUnsignedShort,
             DOC(kp, Memory, DataTypes, eUnsignedShort))
      .value("char",
             kp::Memory::DataTypes::eChar,
             DOC(kp, Memory, DataTypes, eChar))
      .value("uchar",
             kp::Memory::DataTypes::eUnsignedChar,
             DOC(kp, Memory, DataTypes, eUnsignedChar))
      .export_values();

    py::enum_<kp::Memory::MemoryTypes>(m, "MemoryTypes")
      .value("device",
             kp::Memory::MemoryTypes::eDevice,
             DOC(kp, Memory, MemoryTypes, eDevice))
      .value("host",
             kp::Memory::MemoryTypes::eHost,
             DOC(kp, Memory, MemoryTypes, eHost))
      .value("storage",
             kp::Memory::MemoryTypes::eStorage,
             DOC(kp, Memory, MemoryTypes, eStorage))
      .export_values();

    py::class_<kp::OpBase, std::shared_ptr<kp::OpBase>>(
      m, "OpBase", DOC(kp, OpBase));

    py::class_<kp::OpSyncDevice,
               kp::OpBase,
               std::shared_ptr<kp::OpSyncDevice>>(
      m, "OpSyncDevice", DOC(kp, OpSyncDevice))
      .def(py::init<const std::vector<std::shared_ptr<kp::Memory>>&>(),
           DOC(kp, OpSyncDevice, OpSyncDevice));

    py::class_<kp::OpSyncLocal,
               kp::OpBase,
               std::shared_ptr<kp::OpSyncLocal>>(
      m, "OpSyncLocal", DOC(kp, OpSyncLocal))
      .def(py::init<const std::vector<std::shared_ptr<kp::Memory>>&>(),
           DOC(kp, OpSyncLocal, OpSyncLocal));

    py::class_<kp::OpCopy, kp::OpBase, std::shared_ptr<kp::OpCopy>>(
      m, "OpCopy", DOC(kp, OpCopy))
      .def(py::init<const std::vector<std::shared_ptr<kp::Memory>>&>(),
           DOC(kp, OpCopy, OpCopy));

    py::class_<kp::OpAlgoDispatch,
               kp::OpBase,
               std::shared_ptr<kp::OpAlgoDispatch>>(
      m, "OpAlgoDispatch", DOC(kp, OpAlgoDispatch))
      .def(py::init<const std::shared_ptr<kp::Algorithm>&,
                    const std::vector<float>&>(),
           DOC(kp, OpAlgoDispatch, OpAlgoDispatch),
           py::arg("algorithm"),
           py::arg("push_consts") = std::vector<float>())
      .def(py::init(&opAlgoDispatchPyInit),
           DOC(kp, OpAlgoDispatch, OpAlgoDispatch),
           py::arg("algorithm"),
           py::arg("push_consts"));

    py::class_<kp::OpMult, kp::OpBase, std::shared_ptr<kp::OpMult>>(
      m, "OpMult", DOC(kp, OpMult))
      .def(py::init<const std::vector<std::shared_ptr<kp::Memory>>&,
                    const std::shared_ptr<kp::Algorithm>&>(),
           DOC(kp, OpMult, OpMult));

    py::class_<kp::Algorithm, std::shared_ptr<kp::Algorithm>>(
      m, "Algorithm", DOC(kp, Algorithm, Algorithm))
      .def("get_mem_objects",
           &kp::Algorithm::getMemObjects,
           DOC(kp, Algorithm, getMemObjects))
      .def("destroy", &kp::Algorithm::destroy, DOC(kp, Algorithm, destroy))
      .def("is_init", &kp::Algorithm::isInit, DOC(kp, Algorithm, isInit));

    py::class_<kp::Memory, std::shared_ptr<kp::Memory>>(
      m, "Memory", DOC(kp, Memory));

    py::class_<kp::Tensor, std::shared_ptr<kp::Tensor>, kp::Memory>(
      m, "Tensor", DOC(kp, Tensor))
      .def(
        "data",
        [](kp::Tensor& self) {
            // Non-owning container exposing the underlying pointer
            switch (self.dataType()) {
                case kp::Memory::DataTypes::eFloat:
                    return py::array(
                      self.size(), self.data<float>(), py::cast(&self));
                case kp::Memory::DataTypes::eUnsignedInt:
                    return py::array(
                      self.size(), self.data<uint32_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eInt:
                    return py::array(
                      self.size(), self.data<int32_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eDouble:
                    return py::array(
                      self.size(), self.data<double>(), py::cast(&self));
                case kp::Memory::DataTypes::eBool:
                    return py::array(
                      self.size(), self.data<bool>(), py::cast(&self));
                default:
                    throw std::runtime_error(
                      "Kompute Python data type not supported");
            }
        },
        DOC(kp, Memory, data))
      .def("size", &kp::Tensor::size, DOC(kp, Memory, size))
      .def("__len__", &kp::Tensor::size, DOC(kp, Memory, size))
      .def("memory_type", &kp::Memory::memoryType, DOC(kp, Memory, memoryType))
      .def("data_type", static_cast<kp::Memory::DataTypes (kp::Memory::*)()>(&kp::Memory::dataType), DOC(kp, Memory, dataType))
      .def("is_init", &kp::Tensor::isInit, DOC(kp, Tensor, isInit))
      .def("destroy", &kp::Tensor::destroy, DOC(kp, Tensor, destroy));
    py::class_<kp::Image, std::shared_ptr<kp::Image>, kp::Memory>(
      m, "Image", DOC(kp, Image))
      .def(
        "data",
        [](kp::Image& self) {
            // Non-owning container exposing the underlying pointer
            switch (self.dataType()) {
                case kp::Memory::DataTypes::eFloat:
                    return py::array(
                      self.size(), self.data<float>(), py::cast(&self));
                case kp::Memory::DataTypes::eUnsignedInt:
                    return py::array(
                      self.size(), self.data<uint32_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eInt:
                    return py::array(
                      self.size(), self.data<int32_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eUnsignedShort:
                    return py::array(
                      self.size(), self.data<uint16_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eShort:
                    return py::array(
                      self.size(), self.data<int16_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eUnsignedChar:
                    return py::array(
                      self.size(), self.data<uint8_t>(), py::cast(&self));
                case kp::Memory::DataTypes::eChar:
                    return py::array(
                      self.size(), self.data<int8_t>(), py::cast(&self));
                default:
                    throw std::runtime_error(
                      "Kompute Python data type not supported");
            }
        },
        DOC(kp, Memory, data))
      .def("size", &kp::Image::size, DOC(kp, Memory, size))
      .def("__len__", &kp::Image::size, DOC(kp, Memory, size))
      .def("memory_type", &kp::Memory::memoryType, DOC(kp, Memory, memoryType))
      .def("data_type", static_cast<kp::Memory::DataTypes (kp::Memory::*)()>(&kp::Memory::dataType), DOC(kp, Memory, dataType))
      .def("is_init", &kp::Image::isInit, DOC(kp, Image, isInit))
      .def("destroy", &kp::Image::destroy, DOC(kp, Image, destroy));

    py::class_<kp::Sequence, std::shared_ptr<kp::Sequence>>(m, "Sequence")
      .def(
        "record",
        [](kp::Sequence& self, std::shared_ptr<kp::OpBase> op) {
            return self.record(op);
        },
        DOC(kp, Sequence, record))
      .def(
        "eval",
        [](kp::Sequence& self) { return self.eval(); },
        DOC(kp, Sequence, eval))
      .def(
        "eval",
        [](kp::Sequence& self, std::shared_ptr<kp::OpBase> op) {
            return self.eval(op);
        },
        DOC(kp, Sequence, eval_2))
      .def(
        "eval_async",
        [](kp::Sequence& self) { return self.eval(); },
        DOC(kp, Sequence, evalAwait))
      .def(
        "eval_async",
        [](kp::Sequence& self, std::shared_ptr<kp::OpBase> op) {
            return self.evalAsync(op);
        },
        DOC(kp, Sequence, evalAsync))
      .def(
        "eval_await",
        [](kp::Sequence& self) { return self.evalAwait(); },
        DOC(kp, Sequence, evalAwait))
      .def(
        "eval_await",
        [](kp::Sequence& self, uint32_t wait) { return self.evalAwait(wait); },
        DOC(kp, Sequence, evalAwait))
      .def("is_recording",
           &kp::Sequence::isRecording,
           DOC(kp, Sequence, isRecording))
      .def("is_running", &kp::Sequence::isRunning, DOC(kp, Sequence, isRunning))
      .def("is_init", &kp::Sequence::isInit, DOC(kp, Sequence, isInit))
      .def("clear", &kp::Sequence::clear, DOC(kp, Sequence, clear))
      .def("rerecord", &kp::Sequence::rerecord, DOC(kp, Sequence, rerecord))
      .def("get_timestamps",
           &kp::Sequence::getTimestamps,
           DOC(kp, Sequence, getTimestamps))
      .def("destroy", &kp::Sequence::destroy, DOC(kp, Sequence, destroy));

    py::class_<kp::Manager, std::shared_ptr<kp::Manager>>(
      m, "Manager", DOC(kp, Manager))
      .def(py::init(), DOC(kp, Manager, Manager))
      .def(py::init<uint32_t>(),
           DOC(kp, Manager, Manager_2),
           py::arg("device") = 0)
      .def(py::init<uint32_t,
                    const std::vector<uint32_t>&,
                    const std::vector<std::string>&>(),
           DOC(kp, Manager, Manager_3),
           py::arg("device") = 0,
           py::arg("family_queue_indices") = std::vector<uint32_t>(),
           py::arg("desired_extensions") = std::vector<std::string>())
      .def(py::init<uint32_t,
                    const vk::PhysicalDeviceFeatures&,
                    const std::vector<uint32_t>&,
                    const std::vector<std::string>&>(),
           DOC(kp, Manager, Manager_4),
           py::arg("device") = 0,
           py::arg("desired_features") = vk::PhysicalDeviceFeatures(),
           py::arg("family_queue_indices") = std::vector<uint32_t>(),
           py::arg("desired_extensions") = std::vector<std::string>())
      .def("destroy", &kp::Manager::destroy, DOC(kp, Manager, destroy))
      .def("sequence",
           &kp::Manager::sequence,
           DOC(kp, Manager, sequence),
           py::arg("queue_index") = 0,
           py::arg("total_timestamps") = 0)
      .def(
        "tensor",
        [np](kp::Manager& self,
             const py::array_t<float>& data,
             kp::Memory::MemoryTypes memory_type) {
            const py::array_t<float>& flatdata = np.attr("ravel")(data);
            const py::buffer_info info = flatdata.request();
            KP_LOG_DEBUG("Kompute Python Manager tensor() creating tensor "
                         "float with data size {}",
                         flatdata.size());
            return self.tensor(info.ptr,
                               flatdata.size(),
                               sizeof(float),
                               kp::Memory::DataTypes::eFloat,
                               memory_type);
        },
        DOC(kp, Manager, tensor),
        py::arg("data"),
        py::arg("memory_type") = kp::Memory::MemoryTypes::eDevice)
      .def(
        "tensor_t",
        [np](kp::Manager& self,
             const py::array& data,
             kp::Memory::MemoryTypes memory_type) {
            // TODO: Suppport strides in numpy format
            const py::array& flatdata = np.attr("ravel")(data);
            const py::buffer_info info = flatdata.request();
            KP_LOG_DEBUG("Kompute Python Manager creating tensor_T with data "
                         "size {} dtype {}",
                         flatdata.size(),
                         std::string(py::str(flatdata.dtype())));
            if (flatdata.dtype().is(py::dtype::of<std::float_t>())) {
                return self.tensor(info.ptr,
                                   flatdata.size(),
                                   sizeof(float),
                                   kp::Memory::DataTypes::eFloat,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::uint32_t>())) {
                return self.tensor(info.ptr,
                                   flatdata.size(),
                                   sizeof(uint32_t),
                                   kp::Memory::DataTypes::eUnsignedInt,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::int32_t>())) {
                return self.tensor(info.ptr,
                                   flatdata.size(),
                                   sizeof(int32_t),
                                   kp::Memory::DataTypes::eInt,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::double_t>())) {
                return self.tensor(info.ptr,
                                   flatdata.size(),
                                   sizeof(double),
                                   kp::Memory::DataTypes::eDouble,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<bool>())) {
                return self.tensor(info.ptr,
                                   flatdata.size(),
                                   sizeof(bool),
                                   kp::Memory::DataTypes::eBool,
                                   memory_type);
            } else {
                throw std::runtime_error(
                  "Kompute Python no valid dtype supported");
            }
        },
        DOC(kp, Manager, tensorT),
        py::arg("data"),
        py::arg("memory_type") = kp::Memory::MemoryTypes::eDevice)
      .def(
        "image",
        [np](kp::Manager& self,
             const py::array_t<float>& data,
             uint32_t width,
             uint32_t height,
             uint32_t num_channels,
             kp::Memory::MemoryTypes memory_type) {
            const py::array_t<float>& flatdata = np.attr("ravel")(data);
            const py::buffer_info info = flatdata.request();
            KP_LOG_DEBUG("Kompute Python Manager image() creating image "
                         "float with data size {}",
                         flatdata.size());
            return self.image(info.ptr,
                              flatdata.size(),
                              width,
                              height,
                              num_channels,
                              kp::Memory::DataTypes::eFloat,
                              memory_type);
        },
        DOC(kp, Manager, image),
        py::arg("data"),
        py::arg("width"),
        py::arg("height"),
        py::arg("num_channels"),
        py::arg("memory_type") = kp::Memory::MemoryTypes::eDevice)
      .def(
        "image_t",
        [np](kp::Manager& self,
             const py::array& data,
             uint32_t width,
             uint32_t height,
             uint32_t num_channels,
             kp::Memory::MemoryTypes memory_type) {
            // TODO: Suppport strides in numpy format
            const py::array& flatdata = np.attr("ravel")(data);
            const py::buffer_info info = flatdata.request();
            KP_LOG_DEBUG("Kompute Python Manager creating image_T with data "
                         "size {} dtype {}",
                         flatdata.size(),
                         std::string(py::str(flatdata.dtype())));
            if (flatdata.dtype().is(py::dtype::of<std::float_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eFloat,
                                  memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::uint32_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eUnsignedInt,
                                  memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::int32_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eInt,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::uint16_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eUnsignedShort,
                                  memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::int16_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eShort,
                                   memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::uint8_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eUnsignedChar,
                                  memory_type);
            } else if (flatdata.dtype().is(py::dtype::of<std::int8_t>())) {
                return self.image(info.ptr,
                                  flatdata.size(),
                                  width,
                                  height,
                                  num_channels,
                                  kp::Memory::DataTypes::eChar,
                                   memory_type);
            } else {
                throw std::runtime_error(
                  "Kompute Python no valid dtype supported");
            }
        },
        DOC(kp, Manager, imageT),
        py::arg("data"),
        py::arg("width"),
        py::arg("height"),
        py::arg("num_channels"),
        py::arg("memory_type") = kp::Memory::MemoryTypes::eDevice)
      .def(
        "algorithm",
        [](kp::Manager& self,
           const std::vector<std::shared_ptr<kp::Memory>>& tensors,
           const py::bytes& spirv,
           const kp::Workgroup& workgroup,
           const py::list& spec_consts,
           const py::list& push_consts) {
            std::vector<uint32_t> spirvVec = pv::to_vector(spirv);
            KP_LOG_DEBUG("Kompute Python Manager creating Algorithm.");
            auto pushConstsVec = push_consts.cast<std::vector<float>>();
            auto specConstsVec = spec_consts.cast<std::vector<float>>();
            return self.algorithm(
              tensors, spirvVec, workgroup, specConstsVec, pushConstsVec);
        },
        DOC(kp, Manager, algorithm),
        py::arg("tensors"),
        py::arg("spirv"),
        py::arg("workgroup") = kp::Workgroup(),
        py::arg("spec_consts") = py::list(),
        py::arg("push_consts") = py::list())
      .def(
        "algorithm",
        [](kp::Manager& self,
           const std::vector<std::shared_ptr<kp::Memory>>& tensors,
           const py::bytes& spirv,
           const kp::Workgroup& workgroup,
           const py::array& spec_consts,
           const py::list& push_consts) {
            KP_LOG_DEBUG("Kompute Python Manager creating Algorithm_T with "
                         "spec consts data size {} dtype {}",
                         spec_consts.size(),
                         std::string(py::str(spec_consts.dtype())));
            auto pushConstsVec = push_consts.cast<std::vector<float>>();
            if (spec_consts.dtype().is(py::dtype::of<std::float_t>())) {
               return pv::algorithm<float>(self, tensors, spirv, workgroup, spec_consts, pushConstsVec);
            } else if (spec_consts.dtype().is(py::dtype::of<std::int32_t>())) {
              return pv::algorithm<int32_t>(self, tensors, spirv, workgroup, spec_consts, pushConstsVec);
            } else if (spec_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
              return pv::algorithm<uint32_t>(self, tensors, spirv, workgroup, spec_consts, pushConstsVec);
            } else if (spec_consts.dtype().is(py::dtype::of<std::double_t>())) {
              return pv::algorithm<double_t>(self, tensors, spirv, workgroup, spec_consts, pushConstsVec);
            }
            // If reach then no valid dtype supported
            throw std::runtime_error("Kompute Python no valid dtype supported");
        },
        DOC(kp, Manager, algorithm),
        py::arg("tensors"),
        py::arg("spirv"),
        py::arg("workgroup") = kp::Workgroup(),
        py::arg("spec_consts").noconvert(true) = py::array(),
        py::arg("push_consts") = py::list())
        .def(
        "algorithm",
        [](kp::Manager& self,
           const std::vector<std::shared_ptr<kp::Memory>>& tensors,
           const py::bytes& spirv,
           const kp::Workgroup& workgroup,
           const py::list& spec_consts,
           const py::array& push_consts) {
            KP_LOG_DEBUG("Kompute Python Manager creating Algorithm_T with "
                         "push consts data size {} dtype {}",
                         push_consts.size(),
                         std::string(py::str(push_consts.dtype())));
            auto specConstsVec = spec_consts.cast<std::vector<float>>();
            if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
               return pv::algorithm<float>(self, tensors, spirv, workgroup, specConstsVec, push_consts);
            } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
              return pv::algorithm<int32_t>(self, tensors, spirv, workgroup, specConstsVec, push_consts);
            } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
              return pv::algorithm<uint32_t>(self, tensors, spirv, workgroup, specConstsVec, push_consts);
            } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
              return pv::algorithm<double_t>(self, tensors, spirv, workgroup, specConstsVec, push_consts);
            }
            // If reach then no valid dtype supported
            throw std::runtime_error("Kompute Python no valid dtype supported");
        },
        DOC(kp, Manager, algorithm),
        py::arg("tensors"),
        py::arg("spirv"),
        py::arg("workgroup") = kp::Workgroup(),
        py::arg("spec_consts") = py::list(),
        py::arg("push_consts").noconvert(true) = py::array())
      .def(
        "algorithm",
        [np](kp::Manager& self,
             const std::vector<std::shared_ptr<kp::Memory>>& tensors,
             const py::bytes& spirv,
             const kp::Workgroup& workgroup,
             const py::array& spec_consts,
             const py::array& push_consts) {

            KP_LOG_DEBUG("Kompute Python Manager creating Algorithm_T with "
                         "push consts data size {} dtype {} and spec const "
                         "data size {} dtype {}",
                         push_consts.size(),
                         std::string(py::str(push_consts.dtype())),
                         spec_consts.size(),
                         std::string(py::str(spec_consts.dtype())));

            // We have to iterate across a combination of parameters due to the
            // lack of support for templating
            if (spec_consts.dtype().is(py::dtype::of<std::float_t>())) {
                if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
                  return pv::algorithm<float, float>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
                  return pv::algorithm<float, int32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
                  return pv::algorithm<float, uint32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
                  return pv::algorithm<float, double>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                }
            } else if (spec_consts.dtype().is(py::dtype::of<std::int32_t>())) {
                if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
                  return pv::algorithm<int32_t, float>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
                  return pv::algorithm<int32_t, int32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
                  return pv::algorithm<int32_t, uint32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
                  return pv::algorithm<int32_t, double>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                }
            } else if (spec_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
                if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
                  return pv::algorithm<uint32_t, float>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
                  return pv::algorithm<uint32_t, int32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
                  return pv::algorithm<uint32_t, uint32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
                  return pv::algorithm<uint32_t, double>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                }
            } else if (spec_consts.dtype().is(py::dtype::of<std::double_t>())) {
                if (push_consts.dtype().is(py::dtype::of<std::float_t>())) {
                  return pv::algorithm<double, float>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::int32_t>())) {
                  return pv::algorithm<double, int32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::uint32_t>())) {
                  return pv::algorithm<double, uint32_t>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                } else if (push_consts.dtype().is(py::dtype::of<std::double_t>())) {
                  return pv::algorithm<double, double>(self, tensors, spirv, workgroup, spec_consts, push_consts);
                }
            }
            // If reach then no valid dtype supported
            throw std::runtime_error("Kompute Python no valid dtype supported");
        },
        DOC(kp, Manager, algorithm),
        py::arg("tensors"),
        py::arg("spirv"),
        py::arg("workgroup") = kp::Workgroup(),
        py::arg("spec_consts").noconvert(true) = py::array(),
        py::arg("push_consts").noconvert(true) = py::array())
      .def(
        "list_devices",
        [](kp::Manager& self) {
            const std::vector<vk::PhysicalDevice> devices = self.listDevices();
            py::list list;
            for (const vk::PhysicalDevice& device : devices) {
                list.append(kp::py::vkPropertiesToDict(device.getProperties()));
            }
            return list;
        },
        "Return a dict containing information about the device")
      .def(
        "get_device_properties",
        [](kp::Manager& self) {
            const vk::PhysicalDeviceProperties properties =
              self.getDeviceProperties();

            return kp::py::vkPropertiesToDict(properties);
        },
        "Return a dict containing information about the device")
      .def(
        "get_device_features",
        [](kp::Manager& self) {
            return kp::py::vkFeaturesToDict(self.getDeviceFeatures());
        },
        "Return a dict containing information about the device features");

    auto atexit = py::module_::import("atexit");
    atexit.attr("register")(py::cpp_function([]() {
        kp_trace = py::none();
        kp_debug = py::none();
        kp_info = py::none();
        kp_warning = py::none();
        kp_error = py::none();
    }));

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
