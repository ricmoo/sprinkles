let address;
const subgraphUrl = 'https://api.thegraph.com/subgraphs/name/yuetloo/sprinkle-v0-ropsten'
const sprinkleContractAddress = '0x2B240bdA2B9Ce6A9229E40aAbA9D1b0916D00F40';
const sprinkleImageUrl = '/assets/sprinkles.png'
const saveUrl = '/sprinkles';

const selectedUrl = (sprinkleId) => `/sprinkles/${sprinkleId}`

window.onload = () => {
   const connectButton = document.getElementById('connect');
   connectButton.addEventListener('click', handleConnect);

   const homePage = document.getElementById('home');
   const showRoom = document.getElementById('show-room');
   const gallery = document.getElementById('gallery');

   const addressLabel = document.getElementById('address');

   const notFound = document.getElementById('not-found');

   function setAccountAddress(addr) {
      address = addr;
      addressLabel.textContent = formatAdress(addr);
   }

   function formatAdress(addr) {
      return addr.slice(0, 10) + "..." + addr.slice(-8);
   }

   function showNotFound(what) {
      const box = document.createElement('div');
      const message = document.createTextNode(`${what} not found`);
      box.classList.add("not-found");
      box.appendChild(message);
      gallery.appendChild(box);
   }

   function clearSelected() {
      // clear border for previously selected card
      const cards = document.querySelectorAll('.token-card.highlight-border')
      cards.forEach(card => {
         card.className = "token-card";
      })
   }

   function markSelected(element){
      element.classList.add('highlight-border');
   }

   function handleSave(sprinkle, asset, clickedElement) {
      return async (evt) => {

         const json = {
            sprinkle: {
               sprinkleId: sprinkle.id,
               publicKeyHash: sprinkle.publicKeyHash,
               owner: asset.owner,
               tokenId: asset.id,
               tokenContractAddress: asset.contractAddress
            }
          }

         try {
            await ethers.utils.fetchJson(saveUrl, JSON.stringify(json));
            clearSelected();
            markSelected(clickedElement);
         } catch (err) {
            alert(`Error saving config ${err.message}`)
         }

      }
   }

   function fetchSelected(sprinkle) {
      return ethers.utils.fetchJson(selectedUrl(sprinkle.id));
   }

   function showAssets(sprinkle) {
      return async (e) => {
         gallery.textContent="";
         let owner = sprinkle.owner;
         owner = '0x23936429FC179dA0e1300644fB3B489c736D562F';
         const assets = await fetchAssets(owner);

         if( assets.length === 0 ) {
            showNotFound("Assets");
         }

         const selected = await fetchSelected(sprinkle);
         assets.forEach(asset => {
            const tokenElement = buildCard(asset);

            if( asset.id === selected.tokenId &&
                asset.contractAddress === selected.tokenContractAddress
               )
            {
               markSelected(tokenElement);
            }


            gallery.appendChild(tokenElement);
            tokenElement.addEventListener('click', handleSave(sprinkle, asset, tokenElement));
         })

      }
   }

   function gotoDashboard() {
      homePage.style.display = 'none';
      showRoom.style.display = 'block';
      refreshDashboard();
   }

   function refreshDashboard() {
      gallery.textContent =""
      const addr = '0x236ff1e97419ae93ad80cafbaa21220c5d78fb7d';
      showSpinner();
      fetchSprinkles(addr)
         .then(listSprinkles)
         .finally(() => {
            hideSpinner();
         })
   }

   function showSpinner() {

   }

   function hideSpinner() {

   }

   function listSprinkles(sprinkles) {
      if( sprinkles.length === 0 ) {
         notFound.style="display:block";
         return;
      }

      sprinkles.forEach(sprinkle => {
         const tokenElement = buildCard(sprinkle);
         gallery.appendChild(tokenElement);
         tokenElement.addEventListener('click', showAssets(sprinkle));
      })
   }

   function buildCard(token) {
      const box = document.createElement("div");
      box.classList.add("token-card");

      if( token.imageUrl ) {
         const image = document.createElement('img');
         image.src = token.imageUrl;
         image.width = "200";
         image.height = "200";
         image.alt = token.name;
         box.appendChild(image);
      }

      const collection = document.createElement('div');
      const collectionText = document.createTextNode(token.collection);
      collection.appendChild(collectionText);
      collection.classList.add("collection");

      const name = document.createElement('div');
      const nameText = document.createTextNode(token.name);
      name.classList.add("token-name");
      name.appendChild(nameText)

      box.appendChild(collection)
      box.appendChild(name);

      return box;
   }

   async function fetchSprinkles(owner) {
      const json = {
         query: `
         {
            owners (where: { id: "${owner}"}) {
               id
               sprinkles {
                 id
                 owner { id }
                 publicKeyHash
               }
            }
         }
         `
       }

      const res = await ethers.utils.fetchJson(subgraphUrl, JSON.stringify(json));

      let sprinkles = [];

      if( res.data.owners[0] && res.data.owners[0].sprinkles ) {
         sprinkles = res.data.owners[0].sprinkles.map(sprinkle => {
            const tokenId = ethers.BigNumber.from(sprinkle.id).toString();
            return {
               contractAddress: sprinkleContractAddress,
               id: tokenId,
               name: `# ${tokenId}`,
               collection: "Sprinkles",
               owner: sprinkle.owner.id,
               imageUrl: sprinkleImageUrl,
               publicKeyHash: sprinkle.publicKeyHash
            }
         });
      }

      return sprinkles;
   }

   async function fetchAssets(owner) {
      
      const url=`https://api.opensea.io/api/v1/assets?owner=${owner}`
      
      const result = await ethers.utils.fetchJson(url);
      const assets = result.assets.map(asset => ({
         contractAddress: asset.asset_contract.address,
         id: asset.token_id,
         name: asset.name,
         collection: asset.collection.name,
         owner,
         imageUrl: asset.image_url,
         imagePreviewUrl: asset.image_preview_url,
         imageThumbnailUrl: asset.image_thumbnail_url
      }));
      return assets;
   }

   function handleConnect() {
      if (typeof window.ethereum === 'undefined') {
         alert('MetaMask is not installed!');
         return;
      }

      ethereum.request({ method: 'eth_requestAccounts' }).then(async accounts => {
         const provider = new ethers.providers.Web3Provider(window.ethereum);
         provider.ready.then(() => {
            ethereum.on('accountsChanged', (newAccounts) => {
               setAccountAddress(newAccounts[0]);
               refreshDashboard();
            });

            setAccountAddress(accounts[0]);
            gotoDashboard();

         });
      })
   }
};
