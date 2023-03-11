/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include <sc-builder/src/scs_loader.hpp>
#include <sc-agents-common/utils/CommonUtils.hpp>
#include "src/manager/utils/sc_component_utils.hpp"
#include "downloader_handler.hpp"

/**
 * @brief Get class of download node
 * @param context current sc-memory context
 * @param nodeAddr sc-addr of node to download
 * @return Sc-addr of node class if it can be downloaded,
 * return empty sc-addr if class was not found
 */
ScAddr DownloaderHandler::getDownloadableClass(ScMemoryContext * context, ScAddr const & nodeAddr)
{
  ScAddrVector const downloadableClasses = {
      keynodes::ScComponentManagerKeynodes::concept_repository,
      keynodes::ScComponentManagerKeynodes::concept_reusable_component_specification};

  for (ScAddr const & currentClass : downloadableClasses)
  {
    if (context->HelperCheckEdge(currentClass, nodeAddr, ScType::EdgeAccessConstPosPerm))
    {
      return currentClass;
    }
  }

  return ScAddr::Empty;
}

/**
 * @brief Get class of address sc-link
 * @param context current sc-memory context
 * @param linkAddr sc-addr of sc-link with address
 * @return Sc-addr of node class in which sc-link is
 * and this class is supported for downloading,
 * return empty sc-addr if class was not found
 */
ScAddr DownloaderHandler::getUrlLinkClass(ScMemoryContext * context, ScAddr const & linkAddr)
{

  ScAddrVector const downloadableUrls = {
      keynodes::ScComponentManagerKeynodes::concept_github_url,
      keynodes::ScComponentManagerKeynodes::concept_google_drive_url};

  for (ScAddr const & currentClass : downloadableUrls)
  {
    if (context->HelperCheckEdge(currentClass, linkAddr, ScType::EdgeAccessConstPosPerm))
    {
      return currentClass;
    }
  }

  return ScAddr::Empty;
}

// void ReposDownloaderHandler::HandleComponents(
//     ScMemoryContext * context,
//     std::string const & componentPath,
//     std::string & specificationsPath)
// {
//   sc_fs_mkdirs(specificationsPath.c_str());

//   for (auto const & it : m_downloaders)
//   {
//     if (componentPath.find(it.first) != std::string::npos)
//     {
//       ReposDownloader * downloader = it.second;
//       Download(componentPath, specificationsPath, downloader, false);
//       break;
//     }
//   }

//   std::string specificationDirName = GetSpecificationDirName(componentPath, specificationsPath);
//   ScsLoader loader;
//   std::stringstream pathStream;
//   pathStream << specificationDirName << DIRECTORY_DELIMITER << SpecificationConstants::SPECIFICATION_FILENAME;
//   loader.loadScsFile(*context, pathStream.str());
// }

// void ReposDownloaderHandler::HandleRepositories(std::string const & repositoryPath, std::string &
// specificationsPath)
// {
//   sc_fs_mkdirs(specificationsPath.c_str());

//   std::stringstream specificationDirName;

//   for (auto const & it : m_downloaders)
//   {
//     if (repositoryPath.find(it.first) != std::string::npos)
//     {
//       ReposDownloader * downloader = it.second;
//       Download(repositoryPath, specificationsPath, downloader, true);
//       specificationsPath = specificationDirName.str();
//       break;
//     }
//   }
// }

DownloaderHandler::~DownloaderHandler()
{
  for (auto const & it : m_downloaders)
    delete it.second;
}

// void ReposDownloaderHandler::Download(
//     std::string const & path,
//     std::string & specificationsPath,
//     ReposDownloader * downloader,
//     bool isRepository)
// {
//   std::stringstream specificationDirNameStream;
//   std::string specificationDirName = GetSpecificationDirName(path, specificationsPath);
//   sc_fs_mkdirs(specificationDirName.c_str());

//   downloader->Download(path, specificationDirName, isRepository);
// }

void DownloaderHandler::Download(ScMemoryContext * context, ScAddr const & nodeAddr)
{
  ScAddrVector nodeAddressLinkAddrs;
  std::string pathPostfix;
  std::string specificationPostfix;
  std::string url;

  ScAddr const & nodeClassAddr = getDownloadableClass(context, nodeAddr);
  if (!nodeClassAddr.IsValid())
  {
    SC_LOG_ERROR("Can't download. Downloadable class not found");
    return;
  }

  std::string nodeSystIdtf = context->HelperGetSystemIdtf(nodeAddr);
  std::string downloadPath = m_downloadDir + SpecificationConstants::DIRECTORY_DELIMETR + nodeSystIdtf;

  if (!sc_fs_mkdirs(downloadPath.c_str()))
  {
    SC_LOG_ERROR("Can't download. Can't create folder.");
    return;
  };

  // TODO: Optimize choosing get address method
  if (nodeClassAddr == keynodes::ScComponentManagerKeynodes::concept_reusable_component_specification)
  {
    nodeAddressLinkAddrs = componentUtils::SearchUtils::GetSpecificationAddress(context, nodeAddr);
    specificationPostfix = SpecificationConstants::SPECIFICATION_FILENAME;
  }

  if (nodeClassAddr == keynodes::ScComponentManagerKeynodes::concept_repository)
  {
    nodeAddressLinkAddrs = {componentUtils::SearchUtils::GetRepositoryAddress(context, nodeAddr)};
  }

  for (ScAddr const & currentAddressLinkAddr : nodeAddressLinkAddrs)
  {
    ScAddr const & linkAddressClassAddr = getUrlLinkClass(context, currentAddressLinkAddr);
    if (linkAddressClassAddr == keynodes::ScComponentManagerKeynodes::concept_github_url)
    {
      context->GetLinkContent(currentAddressLinkAddr, url);
      pathPostfix = GitHubConstants::SVN_TRUNK + SpecificationConstants::DIRECTORY_DELIMETR + specificationPostfix;

      std::unique_ptr<Downloader> downloader = std::make_unique<DownloaderGit>();
      downloader->Download(url + pathPostfix, downloadPath);
    }
  }
}

// std::string ReposDownloaderHandler::GetSpecificationDirName(
//     std::string const & componentPath,
//     std::string & specificationsPath)
// {
//   std::stringstream specificationDirNameStream;
//   size_t componentDirNameIndex = componentPath.rfind(DIRECTORY_DELIMITER);
//   std::string componentDirName = componentPath.substr(componentDirNameIndex);
//   specificationDirNameStream << specificationsPath << componentDirName;
//   return specificationDirNameStream.str();
// }
