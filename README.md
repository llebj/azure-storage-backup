# azure-storage-backup
`azure-storage-backup` is a simple file backup tool written in .NET that uploads backup archives to Azure blob storage. Some of the key features include:
- [TODO]

# Azure Deployment
## Deployment Script
The Azure resources are deployed by running the `deploy` script found in the `scripts` directory. This script does a couple of things:
1. Creates a resource group (if it does not already exist).
2. Deploys the resources defined in the `main.bicep` file to the specified resource group.

This script takes the following positional parameters:
| Parameter | Description |
| --- | --- |
| `$1` | The name of the resource group |
| `$2` | The path to a `bicepparam` file |

This script depends on the Azure CLI and the `jq` package. It also assumes that a default location has been set. This can be done by running `az configure --defaults location=<location>`.

## Bicep Parameters
The `bicepparam` file must contain a value for the `storage_account_name` parameter.

[TODO]

## Role-Based Access Control Configuration
Once the Azure resources have been deployed an App Registration needs to be created in Entra ID for the backup tool. The corresponding service principal then needs to be assigned a role with permissions to read from and write to the specified storage account. The built-in `Storage Blob Data Contributor` role is sufficient.

# Backup Tool
## Overview
This application is used to take system backups and store them in Azure blob storage. It takes advantage of the copy-on-write and subvolume features of the btrfs filesystem to take immutable backups backups of the filesystem.

It is made up of four major components:
- The `AzureBackupTool` .NET application. This is responsible for detecting new snapshots, creating zipped archives, and uploading them to azure.
- pacman hooks. Pre-install hooks are used to create system snapshots before each system upgrade. A post-install hook is used to clean up old snapshots that have been uploaded by the .NET application.
- A systemd service unit to automatically start the application at system startup.
- bash scripts that are used as part of the backup process and for installation of the application.

### Pacman Hooks
These hooks are responsible for:
- `boot-backup-pre-install` - copies the boot partition into the main filesystem. This should be run before the `btrfs-snapshot-pre-install` hook.
- `btrfs-snapshot-pre-instll` - creates a read-only snapshot of the root subvolume.
- `remove-old-snapshots-post-install` - deletes old snapshots that have been archived and uploaded to Azure. This helps prevent storage usage being taken up by old backups locally.

## Installation
There are four components that need to be installed for this applicaiton to function:
- The `AzureBackupTool` .NET application.
- pacman hooks
- systemd service
- bash scripts

The `install-app` bash script installs the .NET application so that it is installed for all users. It also installs the SQLite DB used to maintain the application state, as well as the settings file used to configura the application (`/etc/azure-storage-backup/appsettings.json` by default).

The `install-service` script installs the `azure-storage-backup.service` file into `/etc/systemd/system`. This service will need to be enabled before the service will start.

Currently the pacman hooks need to be installed manually. This can be done by copying the `boot-backup-pre-install.hook`, `btrfs-snapshot-pre-install.hook`, and `remove-old-snapshots-post-install.hook` files into `/etc/pacman.d/hooks`. 

The following bash scripts must be copied into `/usr/local/bin` as they are executed by the pacman hooks:
- `btrfs-snapshot`
- `remove-old-snapshots`

## Configuration
The application is configured using an `appsettings.json` file. By default the application looks for this file in the directory where the assembly is located. In production environments the app also looks for an optional file in `/etc/azure-storage-backup` that is used for system wide configuration. This system wide configuration file takes precedence over the file stored in the install directory.

The following settings must be defined set:
- `Program:ArchiveOutputDirectory` - The directory used by the application for temporary storage for the archived snapshots before they are uploaded to Azure. Defaults to `/var/lib/azure-storage-backup/archives`.
- `Program:DatabasePath` - The path to the SQLite database file. Defaults to `/var/lib/azure-storage-backup/source.db`.
- `Program:SourceDirectory` - The directory on the filesystem that contains snapshots to backup. The pacman hook uses `/.snapshots`.
- `Program:DestinationContainer` - The name of the Azure container to upload archives into.
- `Program:StorageHosting` - Select whether the program should upload to Azure in the cloud, or a local Azurite instance. Allowed values are: `cloud` and `local`.

The following settings must be defined if `Program:StorageHosting` is set to `cloud`:
- `CloudStorageCredentials:BlobEndpoint` - The endpoint of the blob service to upload to.
- `CloudStorageCredentials:ClientId` - The client ID of the app registration used to authenticate the app against the azure tenant.
- `CloudStorageCredentials:ClientSecret` - A valid secret associated with the authentication client app registration.
- `CloudStorageCredentials:TenantId` - The ID of the tenant to authenticate against.

The following settings must be defined if `Program:StorageHosting` is set to `local`:
- `LocalStorageCredentials:BlobEndpoint` - The endpoint of the blob service to upload to.
- `LocalStorageCredentials:AccountName` - The account name of the storage account. If using Azurite this is the well-know account name.
- `LocalStorageCredentials:AccountKey` - The account key of the storage account. If using Azurite this is the well-know account key.
