#ifndef KALEIDOSCOPE_JIT_HPP
#define KALEIDOSCOPE_JIT_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"

#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/IR/Mangler.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>

#pragma GCC diagnostic pop

namespace Kaleidoscope {

class JIT
{
private:
	using ObjLayerT = llvm::orc::LegacyRTDyldObjectLinkingLayer;
	using CompileLayerT = llvm::orc::LegacyIRCompileLayer<ObjLayerT, llvm::orc::SimpleCompiler>;

	std::vector<llvm::orc::VModuleKey> m_moduleKeys{};
	llvm::orc::ExecutionSession m_executionSession{};
	std::unique_ptr<llvm::TargetMachine> m_targetMachine{llvm::EngineBuilder().selectTarget()};
	const llvm::DataLayout m_dataLayout{m_targetMachine->createDataLayout()};
	std::shared_ptr<llvm::orc::SymbolResolver> m_resolver{
		llvm::orc::createLegacyLookupResolver(
			m_executionSession,
			[this](llvm::StringRef a_name)
			{
				return findMangledSymbol(a_name.str());
			},
			[](llvm::Error a_error)
			{
				llvm::cantFail(std::move(a_error), "lookupFlags failed");
			})};
	ObjLayerT m_objectLayer{
		llvm::AcknowledgeORCv1Deprecation, m_executionSession, [this](llvm::orc::VModuleKey)
		{
			return ObjLayerT::Resources{std::make_shared<llvm::SectionMemoryManager>(), m_resolver};
		}};
	CompileLayerT m_compileLayer{
		llvm::AcknowledgeORCv1Deprecation, m_objectLayer, llvm::orc::SimpleCompiler{*m_targetMachine}};

public:
	JIT() { llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr); }

	llvm::TargetMachine& targetMachine() noexcept { return *m_targetMachine; }

	llvm::orc::VModuleKey addModule(std::unique_ptr<llvm::Module> a_module)
	{
		auto key = m_executionSession.allocateVModule();
		cantFail(m_compileLayer.addModule(key, std::move(a_module)));
		m_moduleKeys.push_back(key);
		return key;
	}

	void removeModule(llvm::orc::VModuleKey a_key)
	{
		m_moduleKeys.erase(llvm::find(m_moduleKeys, a_key));
		cantFail(m_compileLayer.removeModule(a_key));
	}

	llvm::JITSymbol findSymbol(const std::string& a_name)
	{
		return findMangledSymbol(mangle(a_name));
	}

private:
	std::string mangle(const std::string& a_name)
	{
		std::string mangledName;
		{
			llvm::raw_string_ostream mangledNameStream(mangledName);
			llvm::Mangler::getNameWithPrefix(mangledNameStream, a_name, m_dataLayout);
		}
		return mangledName;
	}

	llvm::JITSymbol findMangledSymbol(const std::string& a_name)
	{
#ifdef _WIN32
		// The symbol lookup of ObjectLinkingLayer uses the SymbolRef::SF_Exported
		// flag to decide whether a symbol will be visible or not, when we call
		// IRCompileLayer::findSymbolIn with exportedSymbolsOnly set to true.
		//
		// But for Windows COFF objects, this flag is currently never set.
		// For a potential solution see: https://reviews.llvm.org/rL258665
		// For now, we allow non-exported symbols on Windows as a workaround.
		const bool exportedSymbolsOnly = false;
#else
		const bool exportedSymbolsOnly = true;
#endif

		// Search modules in reverse order: from last added to first added.
		// This is the opposite of the usual search order for dlsym, but makes more
		// sense in a REPL where we want to bind to the newest available definition.
		for (auto key : llvm::make_range(m_moduleKeys.rbegin(), m_moduleKeys.rend())) {
			if (auto symbol = m_compileLayer.findSymbolIn(key, a_name, exportedSymbolsOnly)) {
				return symbol;
			}
		}

		// If we can't find the symbol in the JIT, try looking in the host process.
		if (auto address = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(a_name)) {
			return llvm::JITSymbol(address, llvm::JITSymbolFlags::Exported);
		}

#ifdef _WIN32
		// For Windows retry without "_" at beginning, as RTDyldMemoryManager uses
		// GetProcAddress and standard libraries like msvcrt.dll use names
		// with and without "_" (for example "_itoa" but "sin").
		if (a_name.length() > 2 && a_name[0] == '_') {
			if (auto address = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(a_name.substr(1))) {
				return llvm::JITSymbol(address, llvm::JITSymbolFlags::Exported);
			}
		}
#endif

		return nullptr;
	}
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_JIT_HPP
