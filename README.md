# azure-storage-backup
`azure-storage-backup` is a simple file backup tool written in .NET that uploads backup archives to Azure blob storage. Some of the key features include:
- [TODO]

# Deployment
## Azure Resources
The Azure resources are deployed by running the `.deploy` script found in the `scripts` directory. This script does a couple of things:
1. Creates a resource group (if it does not already exist).
2. Deploys the resources defined in the `main.bicep` file to the specified resource group.

This script takes the following positional parameters:
| Parameter | Description |
| --- | --- |
| `$1` | The name of the resource group |
| `$2` | The name of the storage account |

This script depends on the Azure CLI and the `jq` package. It also assumes that a default location has been set. This can be done by running `az configure --defaults location=<location>`.

Once the Azure resources have been deployed an App Registration needs to be created in Entra ID for the backup tool. The corresponding service principal then needs to be assigned to read and write blob data. The built-in `Storage Blob Data Contributor` role is sufficient.

## Backup Tool
### Connecting to Azure
Both the `Azure` and `BlobContainer` sections of `appsettings.json` have to be configured for the app to upload blobs. `BlobContainer.Name` is set to a default value of `backups` to match the container name that is configured in `main.bicep`. The `Azure` section is used to configure the `BlobServiceClient` that is used to interact with the blob service in Azure. The following values are required:
| Setting | Description |
| --- | --- |
| `BlobEndpoint` | The blob service endpoint URI for the connected storage account |
| `ClientId` | The client ID of the app registration |
| `ClientSecret` | A valid secret associated with the app registration |
| `TenantId` | The ID of the directory (tenant) that hosts the app registration |

### Profiles
[TODO]

### Running the App
The app can be started by running `dotnet run` in the `AzureBackupTool` directory.
