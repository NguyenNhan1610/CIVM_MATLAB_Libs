% Start with a clean slate
clc; clear all; close all;

% Parameters that almost never change.
hdr_off    = 0;         % Typically there is no offset to the header
byte_order = 'ieee-le'; % Assume little endian format
precision = 'int16';      % Can we read this from header? CSI extended mode uses int32

% Reconstruction options
options.headerfilename = filepath();
options.datafilename = '';
options.overgridfactor = 2;
options.nNeighbors = 3;
options.scale = 4;
options.dcf_iter = 25;

% Read header
header = ge_read_header(options.headerfilename, hdr_off, byte_order);

% Store important info from header
npts = header.rdb.rdb_hdr_frame_size;%view points
nframes  = header.rdb.rdb_hdr_user20; %will change to header.rdb.rdb_hdr_user5 once baselines are removed;


fid = fopen(options.headerfilename, 'r', byte_order);
fseek(fid, header.rdb.rdb_hdr_off_data, 'bof');
data = fread(fid,inf,precision);

% Data is complex (real and imaginery parts alternate)
data = complex(data(1:2:end),data(2:2:end));

% Separate baselines from raw data
nframes  = length(data(:))/npts;
data = reshape(data,npts,nframes);% Reshape into matrix

% Remove baselines
skip_frames = header.rdb.rdb_hdr_da_yres; %Changed to improve skip frames (was rdb_hdr_nframes)
data(:, 1:skip_frames:nframes) = []; % Remove baselines (junk views)
nframes  = length(data(:))/npts;

% Remove extra junk view - not sure why this is necessary
data(:,1) = [];

% Split dissolved and gas data
dissolved_fid_data = data(:,1:2:end);
gas_fid_data = data(:,2:2:end);

% Calculate trajectories
nframes = length(dissolved_fid_data(:))/npts;
rad_traj  = calc_radial_traj_distance(header);
primeplus = header.rdb.rdb_hdr_user23;
traj = calc_archimedian_spiral_trajectories(nframes, primeplus, rad_traj)';

% Undo loopfactor from data and trajectories
% (not necessary, but nice if you want to plot fids)
loop_factor = header.rdb.rdb_hdr_user10;
old_idx = 1:nframes;
new_idx = mod((old_idx-1)*loop_factor,nframes)+1;
dissolved_fid_data(:,old_idx) = dissolved_fid_data(:,new_idx);
gas_fid_data(:,old_idx) = gas_fid_data(:,new_idx);
traj = reshape(traj, [npts, nframes 3]);
traj(:,old_idx, :) = traj(:,new_idx,:);
% traj = reshape(traj,[npts*nframes 3]);
clear old_idx new_idx;

% % Temporally recon the data
% start_frame = 1;
% nframes_ = 200;
% dissolved_fid_data = dissolved_fid_data(:,start_frame:(start_frame+nframes_ - 1));
% gas_fid_data = gas_fid_data(:,start_frame:(start_frame+nframes_ - 1));
% traj = traj(:,start_frame:(start_frame+nframes_ - 1),:);
% traj = reshape(traj,[nframes_*npts 3]);
% 
% % Show sampling
% figure();
% plot3(traj(:,1),traj(:,2),traj(:,3),'.b');
% hold on;
% plot3(traj(:,1),traj(:,2),traj(:,3),':r');
% hold off;

% Override trajectories
options.traj = traj(:,1:3);

tic;
% Reconstruct gas phase data
options.data = gas_fid_data(:);
[recon_gas, header, reconObj] = Recon_Noncartesian(options);

% Filter
% recon_vol = FermiFilter(recon_gas,0.1/options.scale, 1.2/options.scale);

%Show output
figure();
imslice(abs(recon_gas),'Gas Phase');

% Now add the reconObject and run the recon for the other files - this 
% additional reconstructions will be faster because we don't have to 
% create the reconstruction object and calculate density compensation. 
% If you are reconstructing multiple similar reconstructions,
% its much more efficient to run the recon once, save the reconObj, then
% run it for future reconstructions using the reconObj.
options.reconObj = reconObj;

% Reconstruct dissolved phase data
options.data = dissolved_fid_data(:);
[recon_dissolved, header, reconObj] = Recon_Noncartesian(options);
recon_time = toc

% Filter
% recon_vol = FermiFilter(recon_dissolved,0.1/options.scale, 1.2/options.scale);

%Show output
figure();
imslice(abs(recon_dissolved),'Dissolved Phase');

% Save gas volume
nii = make_nii(abs(recon_gas));
save_nii(nii, 'gas.nii', 32);

% Save dissolved volume
nii = make_nii(abs(recon_dissolved));
save_nii(nii, 'dissolved.nii', 32);