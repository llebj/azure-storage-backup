param accessTier string = 'Hot'
param location string = resourceGroup().location

@minLength(3)
@maxLength(24)
param storage_account_name string

param storage_sku string = 'Standard_LRS'

resource storage_account 'Microsoft.Storage/storageAccounts@2024-01-01' = {
  name: storage_account_name
  location: location
  sku: {
    name: storage_sku
  }
  kind: 'StorageV2'
  properties: {
    publicNetworkAccess: 'Enabled'
    minimumTlsVersion: 'TLS1_2'
    allowSharedKeyAccess: true
    accessTier: accessTier
  }
}

resource blob_service 'Microsoft.Storage/storageAccounts/blobServices@2024-01-01' = {
  parent: storage_account
  name: 'default'
  properties: {
    containerDeleteRetentionPolicy: {
      enabled: true
      days: 7
    }
    deleteRetentionPolicy: {
      allowPermanentDelete: false
      enabled: true
      days: 7
    }
    isVersioningEnabled: true
  }
}

resource container 'Microsoft.Storage/storageAccounts/blobServices/containers@2024-01-01' = {
  parent: blob_service
  name: 'backups'
  properties: {
    defaultEncryptionScope: '$account-encryption-key'
    denyEncryptionScopeOverride: false
    publicAccess: 'None'
  }
}
