param access_tier string = 'Hot'
param location string = resourceGroup().location

@allowed([
  'live'
  'test'
])
param policy_definitions string = 'test'

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
    accessTier: access_tier
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

resource test_policies 'Microsoft.Storage/storageAccounts/managementPolicies@2021-04-01' = if (policy_definitions == 'test') {
  name: 'default'
  parent: storage_account
  properties: {
    policy: {
      rules: [
        {
          name: 'delete-old-versions'
          enabled: true
          type: 'Lifecycle'
          definition: {
            filters: {
              blobTypes: ['blockBlob']
            }
            actions: {
              version: {
                delete: {
                  daysAfterCreationGreaterThan: 7
                }
              }
            }
          }
        }
      ]
    }
  }
}

resource policies 'Microsoft.Storage/storageAccounts/managementPolicies@2021-04-01' = if (policy_definitions == 'live') {
  name: 'default'
  parent: storage_account
  properties: {
    policy: {
      rules: [
        {
          name: 'cool-store-versions'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              version: {
                tierToCool: {
                  daysAfterCreationGreaterThan: 7
                }
              }
            }
          }
        }
        {
          name: 'archive-old-versions'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              version: {
                tierToArchive: {
                  daysAfterCreationGreaterThan: 37
                }
              }
            }
          }
        }
        {
          name: 'delete-old-versions'
          enabled: true
          type: 'Lifecycle'
          definition: {
            actions: {
              version: {
                delete: {
                  daysAfterCreationGreaterThan: 365
                }
              }
            }
          }
        }
      ]
    }
  }
}
